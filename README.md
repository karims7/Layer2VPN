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
