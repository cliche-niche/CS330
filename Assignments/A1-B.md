Part B of the first assignment had seven system calls to be implemented-

1. `getppid`: Returns the `pid` of the parent of the calling process. Takes no argument. If the calling process has no parent (for whatever reason), it returns -1. [3 points]

2. `yield`: The calling process is de-scheduled i.e., it voluntarily yields the CPU to other processes. Take help of the `yield()` function defined in [kernel/proc.c](../A1/xv6-riscv/kernel/proc.c). Returns zero always. Takes no argument. [2 points]

3. `getpa`: Takes a virtual address (i.e., a pointer) as the only arguments and returns the corresponding physical address. For example, if `x` is a variable in the calling program, `getpa(&x)` returns the physical address of `x`. Take help of the function `walkaddr()` defined in [kernel/vm.c](../A1/xv6-riscv/kernel/vm.c). Given a virtual address `A` in a process `p`, `walkaddr(p->pagetable, A) + (A & (PGSIZE - 1))` computes the physical address of A. [4 points]

4. `forkf`: This system call introduces a slight variation in the `fork()` call. The usual `fork()` call returns to the code right after the `fork()` call in both parent and child. In `forkf()` call, the parent behaves just like the usual `fork()` call, but the child first executes a function and then returns to the code after the `forkf()` call. The `forkf` system call takes the function address as an argument. It will be helpful to understand how the `fork()` call is implemented and from where the program counter is picked up when a trap returns to user mode.<br>
Consider the following example program that uses `forkf()`. <br>

    ```C
    #include "kernel/types.h"
    #include "user/user.h"

    int g (int x)
    {
       return x*x;
    }

    int f (void)
    {
       int x = 10;

       fprintf(2, "Hello world! %d\n", g(x));
       return 0;
    }

    int
    main(void)
    {
      int x = forkf(f);
      if (x < 0) {
         fprintf(2, "Error: cannot fork\nAborting...\n");
         exit(0);
      }
      else if (x > 0) {
         sleep(1);
         fprintf(1, "%d: Parent.\n", getpid());
         wait(0);
      }
      else {
         fprintf(1, "%d: Child.\n", getpid());
      }

      exit(0);
    }
    ```

    The expected output of this program is shown below.

    ```bash
    Hello world! 100
    4: Child.
    3: Parent.
    ```

    Explain the outputs of the program when the return value of f is 0, 1, and -1. How does the program behave if the return value of f is changed to some integer value other than 0, 1, and -1? How does the program behave if the return type of f is changed to void and the return statement in f is commented? [18 points]

5. `waitpid`: This system call takes two arguments: an integer and a pointer. The integer argument can be a pid or -1. The system call waits for the process with the passed pid to complete provided the pid is of a child of the calling process. If the first argument is -1, the system call behaves similarly to the `wait` system call. The normal return value of this system call is the pid of the process that it has waited on. The system call returns -1 in the case of any error. It will be helpful to see how the wait system call is implemented. Make sure to acquire and release the appropriate locks as is done in the implementation of the wait system call and as mentioned in the comments alongside the definition of the proc structure
in [kernel/proc.h](../A1/xv6-riscv/kernel/). [11 points]

6. `ps`: This system call walks over the process table and prints the following pieces of information for each process that has a state other than `UNUSED`: `pid`, `parent pid`, `state`, `command`, `creation time`, `start time`, `execution time`, `size`. For command, use the name field in the `proc` structure. You need to introduce new fields in the proc structure to record creation time of a process, start time of a process, and end time of a process. For each of these, use the "ticks" variable to get the current time as used in the `sys_uptime()` function in [kernel/sysproc.c](../A1/xv6-riscv/kernel/sysproc.c). The creation time of a process is defined as the time when the process is allocated. The start time of a process is defined as the time when the process is scheduled for the first time (recall that a newly created process starts execution in a special kernel function; find out which function it is). The end time of a process is defined as the time when the process becomes `zombie` by calling `exit()`. When `ps` is called, it is possible that certain processes have not yet terminated. For them, the execution time should be printed as the current time minus the start time, while for those which have already terminated, the execution time should be the end time minus the start time. Use the `sz` field of the `proc` structure to get the size of a process. A sample output is shown below.
    ```bash
    pid=1, ppid=-1, state=sleep, cmd=init, ctime=0, stime=1, etime=101, size=0x0000000000003000
    pid=2, ppid=1, state=sleep, cmd=sh, ctime=1, stime=1, etime=101, size=0x0000000000004000
    pid=4, ppid=2, state=run, cmd=testps, ctime=92, stime=92, etime=10, size=0x0000000000003000
    pid=5, ppid=4, state=zombie, cmd=testps, ctime=92, stime=92, etime=1, size=0x0000000000003000
    ```

    Note that the `init` process has no parent process (because it is the root of the process tree). Therefore, its `ppid` is printed as -1.

    It will be helpful to see how the `procdump()` function is implemented in [kernel/proc.c](../A1/xv6-riscv/kernel/proc.c). However, this function does not acquire/release any locks. Your implementation of `ps` must acquire/release the proper locks as you have done in the `waitpid` implementation.

    The return value of this system call is always zero. [6 points]

7. `pinfo`: This system call is similar to `ps`, but instead of printing the information about all processes, it returns the information about a specific process to the calling program. This system call takes two arguments: an integer and a pointer to a `procstat` structure. The procstat structure should be defined as shown below in a new file [kernel/procstat.h](../A1/xv6-riscv/kernel/procstat.h).

    ```C
    struct procstat {
    int pid;     // Process ID
    int ppid;    // Parent ID
    char state[8];  // Process state
    char command[16]; // Process name
    int ctime;    // Creation time
    int stime;    // Start time
    int etime;    // Execution time
    uint64 size;  // Process size
    };
    ```
    This structure has the same pieces of information that the `ps` system call was printing. The first argument of this system call can be a pid or -1. If it is a pid, then the system call fills the second argument with the aforementioned pieces of information regarding the process having the specified pid. If the first argument is -1, then the system call fills the second argument with the aforementioned pieces of information regarding the calling process. Note that you will need to include [kernel/procstat.h](../A1/xv6-riscv/kernel/procstat.h) in the user program that uses the pinfo system call. Also, you will have to include [kernel/procstat.h](../A1/xv6-riscv/kernel/procstat.h) in [kernel/proc.c](../A1/xv6-riscv/kernel/proc.c). 

    The normal return value of this system call is zero. In case of any error (e.g., invalid pid or invalid pointer, etc.), the return value should be -1. [8 points]

    An example user program that uses `pinfo` is shown below.

    ```C
    #include "kernel/types.h"
    #include "kernel/procstat.h"
    #include "user/user.h"

    int
    main(void)
    {
    struct procstat pstat;

    int x = fork();
    if (x < 0) {
        fprintf(2, "Error: cannot fork\nAborting...\n");
        exit(0);
    }
    else if (x > 0) {
        sleep(1);
        fprintf(1, "%d: Parent.\n", getpid());
        if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
        if (pinfo(x, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
        fprintf(1, "Return value of waitpid=%d\n", waitpid(x, 0));
    }
    else {
        fprintf(1, "%d: Child.\n", getpid());
        if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
    }

    exit(0);
    }
    ```

    The output is shown below.

    ```
    4: Child.
    pid=4, ppid=3, state=run, cmd=testpinfo, ctime=31, stime=31, etime=0, size=0x0000000000003000

    3: Parent.
    pid=3, ppid=2, state=run, cmd=testpinfo, ctime=31, stime=31, etime=1, size=0x0000000000003000
    pid=4, ppid=3, state=zombie, cmd=testpinfo, ctime=31, stime=31, etime=0, size=0x0000000000003000

    Return value of waitpid=4
    ```