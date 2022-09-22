#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Functionality inaugurated by the UG'24@IIT-K

uint64
sys_getppid(void)
{
  // since child is accessing some other process' (parent process') PID, it's lock must be acquired first
  struct proc *p = myproc()->parent;
  if(p == 0) {
    return -1;
  }

  acquire(&(p->lock));
  int ppid = p->pid;
  release(&(p->lock));

  return ppid;
}

uint64
sys_yield(void)
{
  yield();
  return 0;
}

uint64
sys_getpa(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  
  return walkaddr(myproc()->pagetable, p) + (p & (PGSIZE - 1));
}

uint64
sys_forkf(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  
  printf("p = %x\n", p);
  forkf((void*) p);
  return 0;
}

uint64
sys_waitpid(void)
{
  uint64 p,q;
  if(argaddr(0, &p) < 0)
    return -1;
  if(argaddr(1, &q) < 0)
    return -1;
  return waitpid(p,q);
}