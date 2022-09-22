Some points to be reviewed later.

1. `getppid()`: To acquire `wait_lock`? If yes, then function needs to be implemented in [proc.c](./xv6-riscv/kernel/proc.c). Currently it exists only in [sysproc.c](./xv6-riscv/kernel/sysproc.c)

2. `yield()`: No idea how to test. Exists in [sysproc.c](./xv6-riscv/kernel/sysproc.c)

3. `getpa()`: Function signature in [user.h](./xv6-riscv/user/user.h) takes `void*` as argument. But should it be `uint64` or something else?
