Team Members and Division of Labor:

Vishesh Srivastava:
Part 1: System-call Tracing
Part 2: Kernel Module
Testing and bug fixes

Dylan Grant:
Part 3: Elevator Scheduler
Testing and bug fixes

Anjali Agrawal:
Part 1: System-call Tracing
Part 2: Kernel Module
Part 3: Elevator Scheduler
Testing and bug fixes

Contents of Tar:
README.txt
Part 1: System-call Tracing
        empty.c, emtpy.trace, part1.c, and part1.trace. 
Part 2: Kernel Module
        Makefile, my_timer.c
Part 3: Elevator Scheduler
        elevator.c, sys_call.c, Makefile
2 Git commit screnshots:

How to Compile:

        Part2: Run make
        Run: insmod my_timer.ko
        Read proc: cd /proc/timer
        rmmod my_timer.ko
        
        Part3: Run make
        Run: insmod elevator.ko
              consumer.c --start
              producer.c <num>
              consumer.c --stop
        Read proc: cd /proc/elevator
        rmmod elevator.ko
        


Known Bugs and Unfinished Portions:

       When stop syscall is called it unloads passengers on elevator floor x, then it sets OFFLINE on floor +-1;
Special Considerations:
        No special considerations.
