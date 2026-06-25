#include "tap_utils.h"
#include "sys_utils.h"
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <pthread.h>


/*
The structure holds all port information 
*/

struct vport_t {
    int tap_file_descriptor; // file descriptor of the TAP device
    int vport_socket_file_descriptor; // socket used to send/receive data; communicate with VSwitch.
    struct sockaddr_in vswitch_address; // VSwitch address
};



int main (int argc, char const *argv[]){

    if (argc != 3) {
        ERROR_PRINT_THEN_EXIT("Usage: vport {server_ip} {server_port}\n");
    }

    const char *server_ip_str = argv[1];
    int server_port = atoi(argv[2]);

    struct vport_t vport;
    vport_init(&vport, server_ip_str, server_port);

    pthread_t up_forwarder;
    if (pthread_create(&up_forwarder, NULL, forward_ether_data_to_vswitch, &vport) != 0 ) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    pthread_t down_forwarder;
    if (pthread_create(&down_forwarder, NULL, forward_ether_data_to_tap, &vport) != 0) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_create: %s\n", strerror(errno));
    }

    if (pthread_join(up_forwarder, NULL) != 0 || pthread_join(down_forwarder, NULL) != 0) {
        ERROR_PRINT_THEN_EXIT("fail to pthread_join: %s\n", strerror(errno));
    }

    return 0;



}

/*
IFNAMSIZ: Max size of interface name. Defined in <net/if.h>, by the system. Usually 16 which is the max size allowed for interface name.

tap_alloc(): Function defined in tap_utils.h to create a TAP device. Returns the file descriptor of the created TAP device.
In Linux, everything (even devices) is treated as a file. A file descriptor is a unique identifier for a file or device. 
It helps your computer keep track of things it can read from or write to—like files, devices, or even network connections.
So tap_file_descriptor acts like a file number that lets you read and write Ethernet frames directly.
tap_alloc returning negative value indicates an error in creating the TAP device. Maybe you do not have permission. 
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
    if (inet_pton(AF_INET, server_ip_string, &vswitch_address.sin_addr) != 1) { // mutates sin_addr
            ERROR_PRINT_THEN_EXIT("fail to inet_pton: %s\n", strerror(errno));
    } 

    vport->tap_file_descriptor = tap_file_descriptor;
    vport->vport_socket_file_descriptor = vport_socket_file_descriptor;
    vport->vswitch_address = vswitch_address;


    printf("[VPort] TAP device name: %s, VSwitch: %s:%d\n", tap_device, server_ip_string, server_port);

}



void *forward_ether_data_to_vswitch(void *raw_vport) {
    
}