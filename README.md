TAP Device:
Imagine your computer has a real network cable that plugs into the Internet.
Now imagine we pretend to have a cable — one that exists only in software.
That’s a TAP device.

When your operating system (like Linux) sends a network packet to that fake cable, it doesn’t go out into the real world.
Instead, it goes to your program (the C program you’re writing).
Your program can look at it, copy it, or send it somewhere else (like across the Internet).

So TAP is like saying:

“Pretend this computer has a real Ethernet card — but instead of going to the wall, it goes into my program.”

You’ll have two computers:

Each one runs your C program (vport).

Each vport creates its own TAP device (say ShadabsTap).

The OS thinks each TAP is a local network card.

But behind the scenes, your program sends frames over UDP to your Python VSwitch (running on another machine).

The switch passes frames around, making it look like the two TAPs are on the same Ethernet network.

So your TAP is the fake network cable connected to the real OS,
and your UDP socket is the fake cable connected to the virtual switch on the Internet.

## Physical Analogy for This Virtual Switch Project

Imagine this project as if it were a real physical network installation.

### The real-world setup

- Each computer is like a room.
- Each room has a wall socket for network connection.
- Each room is wired to a central switch in a closet.
- The central switch forwards messages between rooms.

### What each part represents

- `VPort` (`vport.c`) = the room’s network setup
  - It creates a wall socket in the room
  - It connects that socket to a cable going out to the switch
  - It remembers where the switch is

- TAP device = the wall socket in the room
  - The computer thinks this socket is a real network port
  - The socket is named `ShadabsTap`
  - The computer can send and receive network traffic through it

- `vport_init()` = installing the socket and wiring the room
  - It “creates” the wall socket
  - It “plugs” the room into the central switch
  - It stores the switch location and connection details

- UDP socket = the physical cable from the room to the switch
  - This is the actual path used to carry traffic
  - It is the wire that travels between the room and the switch closet

- `VSwitch` (`vswitch.py`) = the central switch in the closet
  - It listens for messages coming from all room cables
  - It learns which room owns which device address
  - It forwards traffic down the correct room cable

### How a message travels

1. Computer A sends a packet into its TAP socket.
2. The TAP socket passes the packet to the hidden box behind the wall (`VPort`).
3. `VPort` sends the packet over the UDP cable to the central switch.
4. The switch checks the packet’s destination label.
5. The switch sends the packet down the cable to the correct room.
6. Computer B receives the packet through its TAP socket.

### Why this is a good analogy

- `TAP` is the fake wall socket.
- `VPort` is the hidden wiring box behind the wall.
- `UDP socket` is the cable to the switch.
- `VSwitch` is the switch in the closet.
- The OS and applications behave as if they are connected to a real local network.

### In one sentence

This project is like putting a fake wall network socket in each room, wiring each room to a central switch, and using that switch to let remote computers talk to each other as if they were on the same local network.

### Suggested README section

You can add a section like this to your README:

```markdown
## Physical Analogy

Imagine each computer as a room in a building.

- The TAP device is a wall network socket inside the room.
- `VPort` is the wiring box behind the wall that connects that socket to a cable.
- The UDP socket is the cable running from the room to the central switch.
- `VSwitch` is the switch in the closet that connects all the rooms together.

So, when one computer sends data:

1. the data enters the TAP socket,
2. `VPort` sends it through the cable,
3. the switch receives it,
4. and the switch forwards it to the right remote room.

This makes remote computers behave as if they were plugged into the same physical Ethernet switch.
```
