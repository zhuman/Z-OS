# Z-OS

Z-OS is a little toy OS I wrote when I was 16. It's quite tiny and targets 16-bit and 32-bit [PIC microcontrollers](https://www.microchip.com/). It's all original code exceot for the thread context switching, which was copied from FreeRTOS (since I didn't want to write the assembly that just copies every register myself at risk of missing one). It supports pre-emptive multi-tasking, but it does not implement memory separation or virtual memory.

The design of the OS is heavily inspired by the Windows NT kernel. The code is organized into a series of extensible objects, which form the API. Each object provides COM-like interfaces and are centrally registered with the Object Manager.

The Object Manager manages the root namespace for all named objects on the system (objects don't have to be named though). Similar to NT, objects can themselves can implement namespace parsing, which allows implementing file systems. There is also a generalized abstraction for implementing a file system on top of a block device, complete with a centralized Cache Manager.

The OS currently has implementations of an SD card block device, as well as a FAT filesystem implementation. The OS allows these components to work together through the abstractions, and in turn allows "apps" to just address the namespace rather than needing to know about the underlying file system or block devices (just like a real OS! :-P).
