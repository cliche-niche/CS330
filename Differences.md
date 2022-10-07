Contains differences in files in this repository from those in official solutions.

### Assignment 1

[User files](./xv6-riscv/user)
`atoi` used instead of making custom functions in all files.
+ [forksleep.c](./xv6-riscv/user/forksleep.c): More or less the same
+ [pipeline.c](./xv6-riscv/user/pipeline.c): More or less the same, except that `write` takes place before forking
+ [primefactors.c](./xv6-riscv/user/primefactors.c): Same except for recursive/iterative implementation, and writing before forking
+ [uptime.c](./xv6-riscv/user/uptime.c): Does not include `#include "kernel/stat.h"` and `main` accepts arguments in the official solution
+ [user.h](./xv6-riscv/user/user.h): Our solution implemented `getpa` to return `int` instead of `uint64`
+ [usys.pl](./xv6-riscv/user/usys.pl): No difference

[Makefile](./xv6-riscv/Makefile): Same except new additions made for A2