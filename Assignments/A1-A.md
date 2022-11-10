Part A of the first assignment had four user programs to be implemented-

1. [uptime.c](../A1/xv6-riscv/user/uptime.c): Write a program to print the up time of the xv6 virtual machine. Use the wrapper around the uptime system call. Add `$U/_uptime` to `UPROGS` list in [xv6-riscv/Makefile](../A1/xv6-riscv/Makefile). If you run "make qemu" without double quotes, it will compile `uptime.c` and bring up the virtual machine. Now if you run `ls`, you should see uptime in that list. You can test your implementation by running uptime at the `$` prompt of xv6. [5 points]

2. [forksleep.c](../A1/xv6-riscv/user/forksleep.c): This program takes two integers `m` and `n` as command line arguments. `m` can take any positive integer value and `n` can be 0 or 1. You should check these conditions. The main process forks a child. If `n` is zero, the child process should sleep for `m` ticks (achieved by calling `sleep(m)`) and then should print out "pid: Child." without the double quotes where pid can be obtained by calling `getpid()`. In this case, the parent process should print out "pid: Parent." without the double quotes immediately after returning from `fork()`. If `n` is one, the parent process should sleep for `m` ticks after returning from `fork()` and then print out "pid: Parent.", while in this case, the child process should print out "pid: Child." immediately without sleeping. As usual, test your program by booting the virtual machine and running forksleep at the `$` prompt. You should see the prints appearing in opposite orders for two different input values of `n`. Two sample outputs are shown below. [8 points]

    ```bash
    $ forksleep 10 0
    3: Parent.
    4: Child.
    $
    ```

    ```bash
    $ forksleep 10 1
    6: Child.
    5: Parent.
    $
    ```

3. [pipeline.c](../A1/xv6-riscv/user/pipeline.c): This program takes two integers `n` and `x` as command line arguments. `n` must be a positive integer, while `x` can take any integer value. You should check these. The program creates a pipeline of `n` processes (including the top-level process that starts the `main` function). Each process adds its `pid` to `x` and passes the new value of `x` to the next process in the pipeline using a `pipe`. Each process, after incrementing `x`, prints "pid: x". Make sure to close a file descriptor after you are done using it; otherwise you will run out of file descriptors for large `n`. Also, ensure that a process waits for all its children to complete and apply this rule recursively. Two sample outputs are given below. [12 points]

    ```bash
    $ pipeline 10 0
    7: 7
    8: 15
    9: 24
    10: 34
    11: 45
    12: 57
    13: 70
    14: 84
    15: 99
    16: 115
    $
    ```

    ```bash
    $ pipeline 1 100
    18: 118
    $
    ```

4. [primefactors.c](../A1/xv6-riscv/user/primefactors.c): This program takes an integer `n` in the range [2, 100] as command line argument and prints the prime factors of `n` with multiplicity. You may assume that a global
array of all primes in the range [2, 100] is given to you as follows.<br>
    ```C
    int primes[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};
    ```
    The program creates a pipeline of processes. The first process (top-level) divides `n` as many times as possible by 2, passes on the end-result to the next process through a pipe, and prints out 2 those many times followed by its pid. The next process divides n as many times as possible by the next prime number from the primes array, passes on the end-result to the next process through a pipe, and prints out that prime number those many times followed by its `pid`. The procedure continues until `n` drops to 1. A few sample outputs are shown below. In each line, the number within the square brackets is the pid of the process responsible for printing that line. [15 points]

    ```bash
    $ primefactors 90
    2, [3]
    3, 3, [4]
    5, [5]
    $
    ```

    ```bash
    $ primefactors 65
    5, [17]
    13, [20]
    $
    ```