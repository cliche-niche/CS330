Contains differences in files in this repository from those in official solutions.

### Assignment 1

[Kernel files](./xv6-riscv/kernel)
+ [defs.h](./xv6-riscv/kernel/defs.h): `getppid` not present in official solution because it is implemented completely in [sysproc.c](./xv6-riscv/kernel/sysproc.c)
+ [param.h](./xv6-riscv/kernel/param.h): `FSSIZE` changed from 1000 to 2000
+ [proc.c](./xv6-riscv/kernel/proc.c): A lot of differences. Significant differences have been completely replaced by the official solution, with both the official solution and our solution tagged with comments
+ [sysproc.c](./xv6-riscv/kernel/): Different implementation of `getppid` (without acquiring locks); other than that, more or less the same (minor difference in `waitpid` and `pinfo`)
+ [proc.h](./xv6-riscv/kernel/proc.h), [procstat.h](./xv6-riscv/kernel/procstat.h), [syscall.c](./xv6-riscv/kernel/syscall.c), [syscall.h](./xv6-riscv/kernel/syscall.h): No difference

[User files](./xv6-riscv/user)<br>
`atoi` used instead of making custom functions in all files.
+ [forksleep.c](./xv6-riscv/user/forksleep.c): More or less the same
+ [pipeline.c](./xv6-riscv/user/pipeline.c): More or less the same, except that `write` takes place before forking
+ [primefactors.c](./xv6-riscv/user/primefactors.c): Same except for recursive/iterative implementation, and writing before forking
+ [uptime.c](./xv6-riscv/user/uptime.c): Does not include `#include "kernel/stat.h"` and `main` accepts arguments in the official solution
+ [user.h](./xv6-riscv/user/user.h): Our solution implemented `getpa` to return `int` instead of `uint64`
+ [usys.pl](./xv6-riscv/user/usys.pl): No difference

[Makefile](./xv6-riscv/Makefile): Same except new additions made for A2