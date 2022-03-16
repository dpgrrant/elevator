# Project2
This project introduces us to the nuts and bolts of system calls, kernel programming, concurrency, and
synchronization in the kernel. It is divided into three parts. 

Part-1

To find the process id of and return it; we use getpid(), it returns the caller process's process ID (PID).


Part -2 
A kernel module that keeps track of how long it's been since the UNix. Only the value of xtime is kept on the initial proc read. The difference between the current and previous xtime values is likewise saved on all subsequent proc readings.

A basic calendar time, or an elapsed time, with sub-second resolution is represented by struct timespec. It has the following members: time t tv sec, which are declared in time.h.
The proc file system provides access to the kernel's internal data structures for executing processes. It may also be used to get information about the kernel and alter kernel parameters at runtime in Linux (sysctl).
Inserting modules into the kernel is done with the insmod command. Kernel modules are commonly used to add support for new hardware (in the form of device drivers) and/or filesystems, as well as to add system calls. The kernel object file (.ko) is inserted using this command.
insmod [file name] [module-options...]
A kernel module is a piece of kernel code that may be loaded into a running kernel and then uninstalled when the functionality is no longer required.


Part-3

A kernel module that simulates an elevator scheduling method by using new syscalls, threading, and locks.
