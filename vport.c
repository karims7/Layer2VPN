#include "tap_utils.h"
#include "sys_utils.h"
#include <assert.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <net/ethernet.h>
#include <pthread.h>

/*
 * The structure holds all port information.
*/
struct vport_t {
    int tap_file_descriptor;             // file descriptor of the TAP device
    int vport_socket_file_descriptor;    // socket used to send/receive data; communicate with VSwitch
    struct sockaddr_in vswitch_address;  // VSwitch address
};

/*
 * IFNAMSIZ: Max size of interface name. Defined in <net/if.h> by the system. Usually 16, which is
 * the max size allowed for an interface name.
 *
 * tap_alloc(): Function defined in tap_utils.h to create a TAP device. Returns the file descriptor
 * of the created TAP device. In Linux, everything (even devices) is treated as a file. A file
 * descriptor is a unique identifier for a file or device. It helps your computer keep track of
 * things it can read from or write to—like files, devices, or even network connections. So
 * tap_file_descriptor acts like a file number that lets you read and write Ethernet frames
 * directly. A negative value from tap_alloc indicates an error in creating the TAP device. Maybe
 * you do not have permission.
 */
void vport_init(struct vport_t *vport, const char *server_ip_string, int server_port) {
    // Creating TAP device:
    char tap_device[IFNAMSIZ] = "ShadabsTap";
    int tap_file_descriptor = tap_alloc(tap_device);
    int vport_socket_file_descriptor = socket(AF_INET, SOCK_DGRAM, 0);

    if (tap_file_descriptor < 0) {
        ERROR_PRINT_THEN_EXIT("Failed to tap_alloc: %s\n", strerror(errno));
    }
    if (vport_socket_file_descriptor < 0) {
        ERROR_PRINT_THEN_EXIT("Failed to create socket: %s\n", strerror(errno));
    }

    struct sockaddr_in vswitch_address;
    memset(&vswitch_address, 0, sizeof(vswitch_address));
    vswitch_address.sin_family = AF_INET;
    vswitch_address.sin_port = htons(server_port); // storing port number in network byte order

    // inet_pton() converts an IP address from text to binary form.
    if (inet_pton(AF_INET, server_ip_string, &vswitch_address.sin_addr) != 1) {
        // Mutates sin_addr.
        ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s\n", strerror(errno));
    }

    vport->tap_file_descriptor = tap_file_descriptor;
    vport->vport_socket_file_descriptor = vport_socket_file_descriptor;
    vport->vswitch_address = vswitch_address;

    printf("[VPort] TAP device name: %s, VSwitch: %s:%d\n", tap_device, server_ip_string, server_port);
}

void *forward_ether_data_to_vswitch(void *raw_vport_pointer) {
    struct vport_t *vport = (struct vport_t *)raw_vport_pointer;

    // Buffer to hold one Ethernet frame. Usually max size is 1518 bytes.
    char ether_data[ETHER_MAX_LEN];

    while (true) {
        // Never stop listening for frames.
        int ethernet_frame_size =
            read(vport->tap_file_descriptor, ether_data, sizeof(ether_data));

        // Only continue if we actually received data.
        if (ethernet_frame_size > 0) {
            // Ethernet header is 14 bytes.
            assert(ethernet_frame_size >= 14);

            const struct ether_header *ethernet_header =
                (const struct ether_header *)ether_data;

            /*
             * Forward Ethernet frame to VSwitch.
             *
             * vport->vport_socket_file_descriptor:
             *   UDP socket used to send data to the Python VSwitch.
             *
             * ether_data:
             *   Actual Ethernet frame bytes.
             *
             * ethernet_frame_size:
             *   Number of bytes in the Ethernet frame.
             *
             * 0:
             *   Send normally. No special flags.
             *
             * (struct sockaddr *)&vport->vswitch_address:
             *   Python VSwitch address. Cast is needed because sendto()
             *   expects a generic socket address type.
             *
             * sizeof(vport->vswitch_address):
             *   Size of the address structure.
             */
            ssize_t sent_byte_count = sendto(
                vport->vport_socket_file_descriptor, ether_data, ethernet_frame_size, 0,
                (struct sockaddr *)&vport->vswitch_address, sizeof(vport->vswitch_address));
            if (sent_byte_count != ethernet_frame_size)
            {
                fprintf(
                    stderr,
                    "sendto size mismatch: ethernet_frame_size=%d, sent_byte_count=%zd\n",
                    ethernet_frame_size,
                    sent_byte_count);
            }

            printf(
                "[VPort] Sent to VSwitch:"
                " dhost<%02x:%02x:%02x:%02x:%02x:%02x>"
                " shost<%02x:%02x:%02x:%02x:%02x:%02x>"
                " type<%04x>"
                " datasz=<%d>\n",

                ethernet_header->ether_dhost[0],
                ethernet_header->ether_dhost[1],
                ethernet_header->ether_dhost[2],
                ethernet_header->ether_dhost[3],
                ethernet_header->ether_dhost[4],
                ethernet_header->ether_dhost[5],

                ethernet_header->ether_shost[0],
                ethernet_header->ether_shost[1],
                ethernet_header->ether_shost[2],
                ethernet_header->ether_shost[3],
                ethernet_header->ether_shost[4],
                ethernet_header->ether_shost[5],

                ntohs(ethernet_header->ether_type),
                ethernet_frame_size);
        }
    }
}

void *forward_ether_data_to_tap(void *raw_vport_pointer) {
    struct vport_t *vport = (struct vport_t *)raw_vport_pointer;
    char ether_data[ETHER_MAX_LEN];
    size_t ether_data_size = sizeof(ether_data);

    // Sender's address:
    struct sockaddr_in sender_address;
    
    while (true) {
        // How much space is available for that address:
        socklen_t sender_address_size = sizeof(vport->vswitch_address);

        ssize_t ethernet_frame_size = recvfrom(
            vport->vport_socket_file_descriptor,  // UDP socket
            ether_data,                           // contents
            ether_data_size,                      // content size (1518 bytes)
            0,                                    // no special instructions
            (struct sockaddr *)&sender_address,
            &sender_address_size);
            

        if (ethernet_frame_size > 0) {
            assert(ethernet_frame_size >= 14);

            const struct ether_header *ether_header = (const struct ether_header *)ether_data;
            ssize_t written_byte_count =
                write(vport->tap_file_descriptor,  // TAP device
                      ether_data, ethernet_frame_size);

            if (written_byte_count != ethernet_frame_size) {
                fprintf(stderr,
                        "write size mismatch: ethernet_frame_size=%zd, written_byte_count=%zd\n",
                        ethernet_frame_size, written_byte_count);
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        ERROR_PRINT_THEN_EXIT("Usage: vport {server_ip} {server_port}\n");
    }

    const char *server_ip_str = argv[1];
    int server_port = atoi(argv[2]);

    struct vport_t vport;
    vport_init(&vport, server_ip_str, server_port);

    pthread_t up_forwarder;
    if (pthread_create(&up_forwarder, NULL, forward_ether_data_to_vswitch, &vport) != 0) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    pthread_t down_forwarder;
    if (pthread_create(&down_forwarder, NULL, forward_ether_data_to_tap, &vport) != 0) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    if (pthread_join(up_forwarder, NULL) != 0 ||
        pthread_join(down_forwarder, NULL) != 0) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_join: %s\n", strerror(errno));
    }

    return 0;
}