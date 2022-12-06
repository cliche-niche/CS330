This repository contains design exercises/assignment submissions made in the course CS330A (2022-23 I Sem.) in a team of four (Team Name: amogOS; Team Members: [Soham Samaddar](https://github.com/CrypthiccCrypto), [Samarth Arora](https://github.com/Samadeol), [Sarthak Kohli](https://github.com/SARTHAK811), and me, [Aditya Tanwar](https://github.com/cliche-niche)).
<br>

The assignments required us to implement programs, syscalls, scheduling algorithms, synchronization primitives, etc. in the xv6 OS evironment. Each of them have a `.md` of their own, describing the assignments (not verbatim, there are slight omissions like submission instructions, overviews, etc.), which can be found in the [assignments](./Assignments/) directory.<br>
There is also a report corresponding to each assignment, which can be found in the [reports](./Reports/) directory.

+ [Assignment#1-Part A](./Assignments/A1-A.md): Implement user programs which use syscalls like `getpid()`, `fork()`, `pipe()`, etc.
+ [Assignment#1-Part B](./Assignments/A1-B.md): Implement syscalls like `getppid()`, `waitpid()`, custom implementation of `fork()`, process info, etc. <br> Full marks were awarded for Assignment#1.
+ [Assignment#2](./Assignments/A2.md): Implement four scheduling algorithms: non-preemptive FCFS, non-preemptive SJF, preemptive RR, and preemptive UNIX scheduler, and compare them using some statistics. <br> 17 marks were deducted in total with the remark:
  > Completion time wrong. (-2) <br>
  > SJF implementation buggy: Burst estimates are wrong. (-15)
+ [Assignment#3](./Assignments/A3.md): Implement `Condition Variables` and `Semaphores` and use them in the [barrier problem](https://en.wikipedia.org/wiki/Barrier_(computer_science)) and the [MP-MC BB problem](https://en.wikipedia.org/wiki/Producer%E2%80%93consumer_problem) (Multiple Producers-Multiple Consumers Bounded Buffer Problem). <br> Full marks were awarded for Assignment#3.


#### Acknowledgements
Most of the xv6-riscv directory was provided to us (in each assignment), we do not take credit for it. We have only made some minor changes/ added new programs to it as part of our assignments, and they have been listed in markdown files included in the repository.<br>
The course website used to be [this](https://www.cse.iitk.ac.in/users/mainakc/2022Autumn/lectures330.html/) but it was removed as soon as the semester ended.