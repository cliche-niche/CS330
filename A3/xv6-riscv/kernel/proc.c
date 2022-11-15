#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "procstat.h"
#include "sleeplock.h"
#include "condvar.h"
#include "semaphore.h"
#include "barrier.h"
#include "buffer.h"

int sched_policy;

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

//Stats
// static int batch_start = 0x7FFFFFFF;
// static int batchsize = 0;
// static int batchsize2 = 0;
// static int turnaround = 0;
// static int completion_tot = 0;
// static int waiting_tot = 0;
// static int completion_max = 0;
// static int completion_min = 0x7FFFFFFF;
// static int num_cpubursts = 0;
// static int cpubursts_tot = 0;
// static int cpubursts_max = 0;
// static int cpubursts_min = 0x7FFFFFFF;
// static int num_cpubursts_est = 0;
// static int cpubursts_est_tot = 0;
// static int cpubursts_est_max = 0;
// static int cpubursts_est_min = 0x7FFFFFFF;
// static int estimation_error = 0;
// static int estimation_error_instance = 0;

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// ####################### Implemented by UG IITK'24 #######################

struct barrier barriers[NUM_BARRIERS];  // La'barrieur arreiuy
struct sleeplock barrier_lock;                // used when accessing the barrier array

struct cond_buffer cond_buffers[NUM_COND_BUFFERS] = {0};
struct sleeplock cond_bufferinsert_lock;
struct sleeplock cond_bufferdelete_lock;

int cond_buffer_tail = 0;
int cond_buffer_head = 0;

sem_buffer sem_buffers[NUM_SEM_BUFFERS] = {0};
struct semaphore bin_sem_con;
struct semaphore bin_sem_pro;
struct semaphore sem_full;
struct semaphore sem_empty;

int sem_buffer_tail = 0;
int sem_buffer_head = 0;

struct sleeplock releasecondsleep_lock;       // used to prevent deadlocks when multiple processes execute condsleep
struct sleeplock print_lock;                  // used to print statements without jumbling up the output
// ####################### Implemented by UG IITK'24 #######################

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl) {
  struct proc *p;
  
  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void
procinit(void)
{
  struct proc *p;
  
  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void) {
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void) {
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid() {
  int pid;
  
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  uint xticks;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);

  p->ctime = xticks;
  p->stime = -1;
  p->endtime = -1;

  p->is_batchproc = 0;
  p->cpu_usage = 0;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

int
forkf(uint64 faddr)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;
  // Make child to jump to function
  np->trapframe->epc = faddr;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
forkp(int priority)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz) < 0){
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  np->base_priority = priority;

  np->is_batchproc = 1;
  np->nextburst_estimate = 0;
  np->waittime = 0;

  release(&np->lock);

  // batchsize++;
  // batchsize2++;

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  np->waitstart = np->ctime;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();
  // uint xticks;

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);
  
  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // acquire(&tickslock);
  // xticks = ticks;
  // release(&tickslock);

  // p->endtime = xticks;

  // if (p->is_batchproc) {

  //    if ((xticks - p->burst_start) > 0) {
  //       num_cpubursts++;
  //       cpubursts_tot += (xticks - p->burst_start);
  //       if (cpubursts_max < (xticks - p->burst_start)) cpubursts_max = xticks - p->burst_start;
  //       if (cpubursts_min > (xticks - p->burst_start)) cpubursts_min = xticks - p->burst_start;
  //       if (p->nextburst_estimate > 0) {
  //          estimation_error += ((p->nextburst_estimate >= (xticks - p->burst_start)) ? (p->nextburst_estimate - (xticks - p->burst_start)) : ((xticks - p->burst_start) - p->nextburst_estimate));
  //          estimation_error_instance++;
  //       }
  //       p->nextburst_estimate = (xticks - p->burst_start) - ((xticks - p->burst_start)*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM + (p->nextburst_estimate*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM;
  //       if (p->nextburst_estimate > 0) {
  //          num_cpubursts_est++;
  //          cpubursts_est_tot += p->nextburst_estimate;
  //          if (cpubursts_est_max < p->nextburst_estimate) cpubursts_est_max = p->nextburst_estimate;
  //          if (cpubursts_est_min > p->nextburst_estimate) cpubursts_est_min = p->nextburst_estimate;
  //       }
  //    }

  //    if (p->stime < batch_start) batch_start = p->stime;
  //    batchsize--;
  //    turnaround += (p->endtime - p->stime);
  //    waiting_tot += p->waittime;
  //    completion_tot += p->endtime;
  //    if (p->endtime > completion_max) completion_max = p->endtime;
  //    if (p->endtime < completion_min) completion_min = p->endtime;
  //    if (batchsize == 0) {
  //       printf("\nBatch execution time: %d\n", p->endtime - batch_start);
	// printf("Average turn-around time: %d\n", turnaround/batchsize2);
	// printf("Average waiting time: %d\n", waiting_tot/batchsize2);
	// printf("Completion time: avg: %d, max: %d, min: %d\n", completion_tot/batchsize2, completion_max, completion_min);
	// if ((sched_policy == SCHED_NPREEMPT_FCFS) || (sched_policy == SCHED_NPREEMPT_SJF)) {
	//    printf("CPU bursts: count: %d, avg: %d, max: %d, min: %d\n", num_cpubursts, cpubursts_tot/num_cpubursts, cpubursts_max, cpubursts_min);
	//    printf("CPU burst estimates: count: %d, avg: %d, max: %d, min: %d\n", num_cpubursts_est, cpubursts_est_tot/num_cpubursts_est, cpubursts_est_max, cpubursts_est_min);
	//    printf("CPU burst estimation error: count: %d, avg: %d\n", estimation_error_instance, estimation_error/estimation_error_instance);
	// }
	// batchsize2 = 0;
	// batch_start = 0x7FFFFFFF;
	// turnaround = 0;
	// waiting_tot = 0;
	// completion_tot = 0;
	// completion_max = 0;
	// completion_min = 0x7FFFFFFF;
	// num_cpubursts = 0;
  //       cpubursts_tot = 0;
  //       cpubursts_max = 0;
  //       cpubursts_min = 0x7FFFFFFF;
	// num_cpubursts_est = 0;
  //       cpubursts_est_tot = 0;
  //       cpubursts_est_max = 0;
  //       cpubursts_est_min = 0x7FFFFFFF;
	// estimation_error = 0;
  //       estimation_error_instance = 0;
  //    }
  // }

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(np = proc; np < &proc[NPROC]; np++){
      if(np->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if(np->state == ZOMBIE){
          // Found one.
          pid = np->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || p->killed){
      release(&wait_lock);
      return -1;
    }
    
    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

int
waitpid(int pid, uint64 addr)
{
  struct proc *np;
  struct proc *p = myproc();
  int found=0;

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for child with pid
    for(np = proc; np < &proc[NPROC]; np++){
      if((np->parent == p) && (np->pid == pid)){
	found = 1;
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        if(np->state == ZOMBIE){
           if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                  sizeof(np->xstate)) < 0) {
             release(&np->lock);
             release(&wait_lock);
             return -1;
           }
           freeproc(np);
           release(&np->lock);
           release(&wait_lock);
           return pid;
	}

        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!found || p->killed){
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct proc *q;
  struct cpu *c = mycpu();
  uint xticks;
  int min_burst, min_prio;
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    if (sched_policy == SCHED_NPREEMPT_SJF) {
       min_burst = 0x7FFFFFFF;
       acquire(&tickslock);
       xticks = ticks;
       release(&tickslock);
       q = 0;
       for(p = proc; p < &proc[NPROC]; p++) {
          acquire(&p->lock);
	  if(p->state == RUNNABLE) {
	     if (!p->is_batchproc) {
                if (q) release(&q->lock);
		q = p;  // Allow main to finish
		break;
	     }
             else if (p->nextburst_estimate < min_burst) {
	        min_burst = p->nextburst_estimate;
		if (q) release(&q->lock);
		q = p;
	     }
             else release(&p->lock);
	  }
	  else release(&p->lock);
       }
       if (q) {
          q->state = RUNNING;
          q->waittime += (xticks - q->waitstart);
          q->burst_start = xticks;
          c->proc = q;
          swtch(&c->context, &q->context);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
	  release(&q->lock);
       }
    }
    else if (sched_policy == SCHED_PREEMPT_UNIX) {
       min_prio = 0x7FFFFFFF;
       acquire(&tickslock);
       xticks = ticks;
       release(&tickslock);
       for(p = proc; p < &proc[NPROC]; p++) {
          acquire(&p->lock);
	  if(p->state == RUNNABLE) {
	     p->cpu_usage = p->cpu_usage/2;
	     p->priority = p->base_priority + (p->cpu_usage/2);
	  }
	  release(&p->lock);
       }
       q = 0;
       for(p = proc; p < &proc[NPROC]; p++) {
          acquire(&p->lock);
          if(p->state == RUNNABLE) {
             if (!p->is_batchproc) {
                if (q) release(&q->lock);
                q = p;  // Allow main to finish
                break;
             }
             else if (p->priority < min_prio) {
                min_prio = p->priority;
                if (q) release(&q->lock);
                q = p;
             }
             else release(&p->lock);
          }
          else release(&p->lock);
       }
       if (q) {
          q->state = RUNNING;
          q->waittime += (xticks - q->waitstart);
          q->burst_start = xticks;
          c->proc = q;
          swtch(&c->context, &q->context);

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
          release(&q->lock);
       }
    }
    else {
       for(p = proc; p < &proc[NPROC]; p++) {
          if ((sched_policy != SCHED_NPREEMPT_FCFS) && (sched_policy != SCHED_PREEMPT_RR)) break;
          acquire(&tickslock);
          xticks = ticks;
          release(&tickslock);
          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            p->state = RUNNING;
	    p->waittime += (xticks - p->waitstart);
	    p->burst_start = xticks;
            c->proc = p;
            swtch(&c->context, &p->context);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
          }
          release(&p->lock);
       }
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  // uint xticks;

  // acquire(&tickslock);
  // xticks = ticks;
  // release(&tickslock);

  acquire(&p->lock);
  p->state = RUNNABLE;
  // p->waitstart = xticks;
  // p->cpu_usage += SCHED_PARAM_CPU_USAGE;
  // if ((p->is_batchproc) && ((xticks - p->burst_start) > 0)) {
  //    num_cpubursts++;
  //    cpubursts_tot += (xticks - p->burst_start);
  //    if (cpubursts_max < (xticks - p->burst_start)) cpubursts_max = xticks - p->burst_start;
  //    if (cpubursts_min > (xticks - p->burst_start)) cpubursts_min = xticks - p->burst_start;
  //    if (p->nextburst_estimate > 0) {
  //       estimation_error += ((p->nextburst_estimate >= (xticks - p->burst_start)) ? (p->nextburst_estimate - (xticks - p->burst_start)) : ((xticks - p->burst_start) - p->nextburst_estimate));
	// estimation_error_instance++;
  //    }
  //    p->nextburst_estimate = (xticks - p->burst_start) - ((xticks - p->burst_start)*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM + (p->nextburst_estimate*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM;
  //    if (p->nextburst_estimate > 0) {
  //       num_cpubursts_est++;
  //       cpubursts_est_tot += p->nextburst_estimate;
  //       if (cpubursts_est_max < p->nextburst_estimate) cpubursts_est_max = p->nextburst_estimate;
  //       if (cpubursts_est_min > p->nextburst_estimate) cpubursts_est_min = p->nextburst_estimate;
  //    }
  // }
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;
  uint xticks;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);

  myproc()->stime = xticks;

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  // uint xticks;

  // if (!holding(&tickslock)) {
  //    acquire(&tickslock);
  //    xticks = ticks;
  //    release(&tickslock);
  // }
  // else xticks = ticks;
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  // p->cpu_usage += (SCHED_PARAM_CPU_USAGE/2);

  // if ((p->is_batchproc) && ((xticks - p->burst_start) > 0)) {
  //    num_cpubursts++;
  //    cpubursts_tot += (xticks - p->burst_start);
  //    if (cpubursts_max < (xticks - p->burst_start)) cpubursts_max = xticks - p->burst_start;
  //    if (cpubursts_min > (xticks - p->burst_start)) cpubursts_min = xticks - p->burst_start;
  //    if (p->nextburst_estimate > 0) {
	// estimation_error += ((p->nextburst_estimate >= (xticks - p->burst_start)) ? (p->nextburst_estimate - (xticks - p->burst_start)) : ((xticks - p->burst_start) - p->nextburst_estimate));
  //       estimation_error_instance++;
  //    }
  //    p->nextburst_estimate = (xticks - p->burst_start) - ((xticks - p->burst_start)*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM + (p->nextburst_estimate*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM;
  //    if (p->nextburst_estimate > 0) {
  //       num_cpubursts_est++;
  //       cpubursts_est_tot += p->nextburst_estimate;
  //       if (cpubursts_est_max < p->nextburst_estimate) cpubursts_est_max = p->nextburst_estimate;
  //       if (cpubursts_est_min > p->nextburst_estimate) cpubursts_est_min = p->nextburst_estimate;
  //    }
  // }

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;
  // uint xticks;

  // if (!holding(&tickslock)) {
  //    acquire(&tickslock);
  //    xticks = ticks;
  //    release(&tickslock);
  // }
  // else xticks = ticks;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
	      // p->waitstart = xticks;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;
  // uint xticks;

  // acquire(&tickslock);
  // xticks = ticks;
  // release(&tickslock);

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
	      // p->waitstart = xticks;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// Print a process listing to console with proper locks held.
// Caution: don't invoke too often; can slow down the machine.
int
ps(void)
{
   static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep",
  [RUNNABLE]  "runble",
  [RUNNING]   "run",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;
  int ppid, pid;
  uint xticks;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->state == UNUSED) {
      release(&p->lock);
      continue;
    }
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

    pid = p->pid;
    release(&p->lock);
    acquire(&wait_lock);
    if (p->parent) {
       acquire(&p->parent->lock);
       ppid = p->parent->pid;
       release(&p->parent->lock);
    }
    else ppid = -1;
    release(&wait_lock);

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);

    printf("pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p", pid, ppid, state, p->name, p->ctime, p->stime, (p->endtime == -1) ? xticks-p->stime : p->endtime-p->stime, p->sz);
    printf("\n");
  }
  return 0;
}

int
pinfo(int pid, uint64 addr)
{
   struct procstat pstat;

   static char *states[] = {
  [UNUSED]    "unused",
  [SLEEPING]  "sleep",
  [RUNNABLE]  "runble",
  [RUNNING]   "run",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;
  uint xticks;
  int found=0;

  if (pid == -1) {
     p = myproc();
     acquire(&p->lock);
     found=1;
  }
  else {
     for(p = proc; p < &proc[NPROC]; p++){
       acquire(&p->lock);
       if((p->state == UNUSED) || (p->pid != pid)) {
         release(&p->lock);
         continue;
       }
       else {
         found=1;
         break;
       }
     }
  }
  if (found) {
     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
         state = states[p->state];
     else
         state = "???";

     pstat.pid = p->pid;
     release(&p->lock);
     acquire(&wait_lock);
     if (p->parent) {
        acquire(&p->parent->lock);
        pstat.ppid = p->parent->pid;
        release(&p->parent->lock);
     }
     else pstat.ppid = -1;
     release(&wait_lock);

     acquire(&tickslock);
     xticks = ticks;
     release(&tickslock);

     safestrcpy(&pstat.state[0], state, strlen(state)+1);
     safestrcpy(&pstat.command[0], &p->name[0], sizeof(p->name));
     pstat.ctime = p->ctime;
     pstat.stime = p->stime;
     pstat.etime = (p->endtime == -1) ? xticks-p->stime : p->endtime-p->stime;
     pstat.size = p->sz;
     if(copyout(myproc()->pagetable, addr, (char *)&pstat, sizeof(pstat)) < 0) return -1;
     return 0;
  }
  else return -1;
}

int
schedpolicy(int x)
{
   int y = sched_policy;
   sched_policy = x;
   return y;
}

// ############################################## implemented by UG - IITK 24 ##############################################

void
condsleep(cond_t* cv, struct sleeplock *lk){
  struct proc *p = myproc();
  // uint xticks;

  // if (!holding(&tickslock)) {
  //    acquire(&tickslock);
  //    xticks = ticks;
  //    release(&tickslock);
  // }
  // else xticks = ticks;
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  // a possible deadlock occurs when mutliple processes 
  // acquire then own lock and then releasesleep lock
  // essentially, two processes hold their own locks whilst trying
  // to obtain the other's lock
  acquiresleep(&releasecondsleep_lock);
  acquire(&p->lock); // There used to be a deadlock here, now its just some lock i used to know
  releasesleep(lk);
  releasesleep(&releasecondsleep_lock);

  // Go to sleep.
  p->chan = (void*) cv;   // sleeping on channel of condition variable
  p->state = SLEEPING;

  // p->cpu_usage += (SCHED_PARAM_CPU_USAGE/2);

  // if ((p->is_batchproc) && ((xticks - p->burst_start) > 0)) {
  //    num_cpubursts++;
  //    cpubursts_tot += (xticks - p->burst_start);
  //    if (cpubursts_max < (xticks - p->burst_start)) cpubursts_max = xticks - p->burst_start;
  //    if (cpubursts_min > (xticks - p->burst_start)) cpubursts_min = xticks - p->burst_start;
  //    if (p->nextburst_estimate > 0) {
	// estimation_error += ((p->nextburst_estimate >= (xticks - p->burst_start)) ? (p->nextburst_estimate - (xticks - p->burst_start)) : ((xticks - p->burst_start) - p->nextburst_estimate));
  //       estimation_error_instance++;
  //    }
  //    p->nextburst_estimate = (xticks - p->burst_start) - ((xticks - p->burst_start)*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM + (p->nextburst_estimate*SCHED_PARAM_SJF_A_NUMER)/SCHED_PARAM_SJF_A_DENOM;
  //    if (p->nextburst_estimate > 0) {
  //       num_cpubursts_est++;
  //       cpubursts_est_tot += p->nextburst_estimate;
  //       if (cpubursts_est_max < p->nextburst_estimate) cpubursts_est_max = p->nextburst_estimate;
  //       if (cpubursts_est_min > p->nextburst_estimate) cpubursts_est_min = p->nextburst_estimate;
  //    }
  // }

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquiresleep(lk);
}

void
wakeupone(void *chan)
{
  struct proc *p;
  // uint xticks;

  // if (!holding(&tickslock)) {
  //    acquire(&tickslock);
  //    xticks = ticks;
  //    release(&tickslock);
  // }
  // else xticks = ticks;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
	      // p->waitstart = xticks;
        release(&p->lock);
        break;
      }
      release(&p->lock);
    }
  }
}

// ################################# Barrier Code ################################# 

int 
barrier_alloc(void) 
{
  for(;;) // if free barrier is not found, loop again
  {
    acquiresleep(&barrier_lock);
    for (int i = 0; i < NUM_BARRIERS; i++)
    {
      if (!barriers[i].pid) {
        barriers[i].pid = myproc()->pid;
        barriers[i].count = 0;
        initsleeplock(&barriers[i].lk, "UG@CSE IITK'24 - Barrier");
        releasesleep(&barrier_lock);
        return i;
      }
    }
    releasesleep(&barrier_lock);
  }
}

void 
barrier(int bin, int barrier_id, int num_proc)
{
  acquiresleep(&(barriers[barrier_id].lk));
  acquiresleep(&print_lock);
  printf("%d: Entered barrier#%d for barrier array id %d\n", myproc()->pid, bin, barrier_id);
  releasesleep(&print_lock);
  barriers[barrier_id].count++;
  if(barriers[barrier_id].count == num_proc) {
    barriers[barrier_id].count = 0;
    cond_broadcast(&barriers[barrier_id].cv);
  }
  else {
    cond_wait(&barriers[barrier_id].cv, &barriers[barrier_id].lk);
  }
  acquiresleep(&print_lock);
  printf("%d: Finished barrier#%d for barrier array id %d\n", myproc()->pid, bin, barrier_id);
  releasesleep(&print_lock);
  releasesleep(&(barriers[barrier_id].lk));
}

void 
barrier_free(int barrier_id)
{
  acquiresleep(&barrier_lock);
  barriers[barrier_id].pid = 0;
  barriers[barrier_id].count = 0;
  releasesleep(&barrier_lock);
}

// ################################# Barrier Code #################################

// ################################# Conditional Buffer Code #################################
void
buffer_cond_init() 
{
  cond_buffer_tail = 0;
  cond_buffer_head = 0;
  initsleeplock(&cond_bufferinsert_lock, "UG@CSE IITK'24 - Buffer Insert");
  initsleeplock(&cond_bufferdelete_lock, "UG@CSE IITK'24 - Buffer Delete");
  for(int i = 0; i < NUM_COND_BUFFERS; i++) {
    initsleeplock(&cond_buffers[i].lk, "UG@CSE IITK'24 - Buffer Lock");
    cond_buffers[i].x = -1;
    cond_buffers[i].full = 0;
  }
}

void
cond_produce(int prod)
{
  int idx = 0;

  acquiresleep(&cond_bufferinsert_lock); // obtain the index where production is to be done
  idx = cond_buffer_tail;
  cond_buffer_tail = (cond_buffer_tail + 1 == NUM_COND_BUFFERS ? 0 : cond_buffer_tail + 1);
  releasesleep(&cond_bufferinsert_lock);

  acquiresleep(&cond_buffers[idx].lk);
  while(cond_buffers[idx].full) cond_wait(&cond_buffers[idx].deleted, &cond_buffers[idx].lk);
  cond_buffers[idx].x = prod;
  cond_buffers[idx].full = 1;
  cond_signal(&cond_buffers[idx].inserted);
  releasesleep(&cond_buffers[idx].lk);
}

int
cond_consume()
{
  int idx = 0;
  int val = 0;

  acquiresleep(&cond_bufferdelete_lock);
  idx = cond_buffer_head;
  cond_buffer_head = (cond_buffer_head + 1 == NUM_COND_BUFFERS ? 0 : cond_buffer_head + 1);
  releasesleep(&cond_bufferdelete_lock);

  acquiresleep(&cond_buffers[idx].lk);
  while(!cond_buffers[idx].full) cond_wait(&cond_buffers[idx].inserted, &cond_buffers[idx].lk);
  val = cond_buffers[idx].x;
  cond_buffers[idx].full = 0;
  cond_signal(&cond_buffers[idx].deleted);
  releasesleep(&cond_buffers[idx].lk);

  acquiresleep(&print_lock);
  printf("%d ", val);
  releasesleep(&print_lock);

  return val;
}
// ################################# Conditional Buffer Code #################################

// ################################# Semaphore Buffer Code #################################
void
buffer_sem_init(void)
{
  sem_buffer_head = 0;
  sem_buffer_tail = 0;
  sem_init(&bin_sem_pro, 1);
  sem_init(&bin_sem_con, 1);
  sem_init(&sem_full, 0);
  sem_init(&sem_empty, NUM_SEM_BUFFERS);
  for (int i = 0; i < NUM_SEM_BUFFERS; i++) sem_buffers[i] = -1;
}

void
sem_produce(int prod)
{
  sem_wait(&sem_empty);
  sem_wait(&bin_sem_pro);
  sem_buffers[sem_buffer_head] = prod;
  sem_buffer_head = (sem_buffer_head + 1 == NUM_SEM_BUFFERS ? 0 : sem_buffer_head + 1);
  sem_post(&bin_sem_pro);
  sem_post(&sem_full);
}

int
sem_consume(void)
{
  int val;
  sem_wait(&sem_full);
  sem_wait(&bin_sem_con);
  val = sem_buffers[sem_buffer_tail];
  sem_buffers[sem_buffer_tail] = -1;
  sem_buffer_tail = (sem_buffer_tail + 1 == NUM_SEM_BUFFERS ? 0 : sem_buffer_tail + 1);
  sem_post(&bin_sem_con);
  sem_post(&sem_empty);

  acquiresleep(&print_lock);
  printf("%d ", val);
  releasesleep(&print_lock);

  return val;
}
// ################################# Semaphore Buffer Code #################################

// ############################################## UG - IITK 24 ##############################################