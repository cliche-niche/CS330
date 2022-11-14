#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)
    return -1;
  return fetchstr(addr, buf, max);
}

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);

extern uint64 sys_getppid(void);
extern uint64 sys_yield(void);
extern uint64 sys_getpa(void);
extern uint64 sys_forkf(void);
extern uint64 sys_waitpid(void);
extern uint64 sys_ps(void);
extern uint64 sys_pinfo(void);
extern uint64 sys_forkp(void);
extern uint64 sys_schedpolicy(void);

// ########################## Adulteration by UG@CSE IITK'24 ##########################
extern uint64 sys_barrier_alloc(void);
extern uint64 sys_barrier(void);
extern uint64 sys_barrier_free(void);

extern uint64 sys_buffer_cond_init(void);
extern uint64 sys_cond_produce(void);
extern uint64 sys_cond_consume(void);

extern uint64 sys_buffer_sem_init(void);
extern uint64 sys_sem_produce(void);
extern uint64 sys_sem_consume(void);
// ########################## Adulteration by UG@CSE IITK'24 ##########################
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,

[SYS_getppid] sys_getppid,
[SYS_yield]   sys_yield,
[SYS_getpa]   sys_getpa,
[SYS_forkf]   sys_forkf,
[SYS_waitpid] sys_waitpid,
[SYS_ps]      sys_ps,
[SYS_pinfo]   sys_pinfo,
[SYS_forkp]   sys_forkp,
[SYS_schedpolicy] sys_schedpolicy,

// ########################## Adulteration by UG@CSE IITK'24 ##########################
[SYS_barrier_alloc]     sys_barrier_alloc,
[SYS_barrier]           sys_barrier,
[SYS_barrier_free]      sys_barrier_free,

[SYS_buffer_cond_init]  sys_buffer_cond_init,
[SYS_cond_produce]      sys_cond_produce,
[SYS_cond_consume]      sys_cond_consume,

[SYS_buffer_sem_init]   sys_buffer_sem_init,
[SYS_sem_produce]       sys_sem_produce,
[SYS_sem_consume]       sys_sem_consume,
// ########################## Adulteration by UG@CSE IITK'24 ##########################
};

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    p->trapframe->a0 = syscalls[num]();
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
