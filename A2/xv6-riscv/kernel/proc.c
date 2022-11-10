#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "procstat.h"

struct cpu cpus[NCPU];


struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

// ################ Added by UG@IITK'24 ################
// stores the current scheduling policy
// default sched policy will be set to 
// RR in main by calling schedpolicy(RR)
int scheduling_policy;

int batch_size = 0;
int batch_size_running = 0;

int batch_start_time = -1;
int batch_end_time = 0;
int batch_execution_time = 0;

int batch_turnaround_time = 0;

int batch_waiting_time = 0;

int batch_completion_time_avg = 0;
int batch_completion_time_min = -1;
int batch_completion_time_max = 0;

int cpu_bursts_count = 0;
int cpu_bursts_avg = 0;
int cpu_bursts_max = 0;
int cpu_bursts_min = -1;

int cpu_est_bursts_count = 0;
int cpu_est_bursts_avg = 0;
int cpu_est_bursts_max = 0;
int cpu_est_bursts_min = -1;

int cpu_bursts_error_count = 0;
int cpu_bursts_error_avg = 0;

void print_statistics(void);
// ################################


extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

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

  // ############# Adulteration by cream of the nation #############
  acquire(&tickslock);
  p->creat_time = ticks;
  release(&tickslock);
  // ##### OFFICIAL SOLUTION #####
  p->start_time = -1;
  p->end_time = -1;
  // ##### OFFICIAL SOLUTION #####
  p->estimate = 0;
  p->base_priority = 0;

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
  np->from_forkp = 0;

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

  // UG-nation strikes again
  acquire(&tickslock);
  p->end_time = ticks;
  release(&tickslock);
  // UG-nation leaves

  if (p->from_forkp) {
    uint xticks;
    if (!holding(&tickslock)) {
       acquire(&tickslock);
       xticks = ticks;
       release(&tickslock);
    }
    else xticks = ticks;
    p->cpu_burst_end_tick = xticks;

    int cpu_burst_length = p->cpu_burst_end_tick - p->cpu_burst_start_tick; // Actual CPU burst length
    if (cpu_burst_length) { // If the actual length is non zero, update the estimate

      // ################## Actual CPU Burst Statistics ##################
      cpu_bursts_count++;
      cpu_bursts_avg += cpu_burst_length;
      if(cpu_burst_length > cpu_bursts_max) cpu_bursts_max = cpu_burst_length;
      if(cpu_bursts_min == -1 || cpu_burst_length < cpu_bursts_min) cpu_bursts_min = cpu_burst_length;

      if(p->estimate) { // If the estimate (BEFORE UPDATING) is also non zero, update the error counts
        // ################## Error CPU Burst Statistics ##################
        cpu_bursts_error_count++;
        if(p->estimate > cpu_burst_length) cpu_bursts_error_avg += p->estimate - cpu_burst_length; // Add the absolute error
        else cpu_bursts_error_avg += cpu_burst_length - p->estimate;
      }

      p->estimate = cpu_burst_length - (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM
                    + (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM; // New estimate to predict the next cpu burst
    }

    if(p->estimate){
      // ################## Estimated CPU Burst Statistics (AFTER UPDATING) ##################
      cpu_est_bursts_count++;
      cpu_est_bursts_avg += p->estimate;
      if(p->estimate > cpu_est_bursts_max) cpu_est_bursts_max = p->estimate;
      if(cpu_est_bursts_min == -1 || cpu_est_bursts_min > p->estimate) cpu_est_bursts_min = p->estimate;
    }

    batch_size_running--;

    batch_turnaround_time += (p->end_time) - (p->creat_time);
    int completion_time = (p->end_time) - (p->start_time);
    batch_completion_time_avg += completion_time;
    if (batch_completion_time_min == -1) {
      batch_completion_time_min = completion_time;
    } else if (batch_completion_time_min > completion_time) {
      batch_completion_time_min = completion_time;
    }

    if (batch_completion_time_max < completion_time) {
      batch_completion_time_max = completion_time;
    }

    if(batch_size_running == 0) {
      uint xticks;
      if (!holding(&tickslock)) {
        acquire(&tickslock);
        xticks = ticks;
        release(&tickslock);
      }
      else xticks = ticks;
      batch_end_time = xticks; 
      batch_execution_time=batch_end_time-batch_start_time;
      print_statistics();
    }
   
  }
  p->xstate = status;
  p->state = ZOMBIE;
  p->estimate = 0;
  p->cpu_burst_start_tick = 0;
  p->cpu_burst_end_tick = 0;
  p->cpu_usage = 0;
  p->base_priority = 0;
  p->dynamic_priority = 0;
  

  release(&wait_lock);

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
  struct cpu *c = mycpu();
  struct proc *next_proc = 0;
  uint xticks;

  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    
    switch (scheduling_policy) 
    {
      case SCHED_NPREEMPT_FCFS: 
        for(p = proc; p < &proc[NPROC]; p++) {
          if (scheduling_policy != SCHED_NPREEMPT_FCFS) break; // Check if scheduling policy changed from FCFS
          
          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            p->state = RUNNING;
            if (p->from_forkp) {
              if (!holding(&tickslock)) {
                acquire(&tickslock);
                xticks = ticks;
                release(&tickslock);
              }
              else xticks = ticks;
              p->runnable_end = xticks;
              batch_waiting_time += (p->runnable_end) - (p->runnable_start);
            }
            c->proc = p;
            swtch(&c->context, &p->context);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
          }
          release(&p->lock);
        }
        break;

      case SCHED_PREEMPT_RR: 
        for(p = proc; p < &proc[NPROC]; p++) {
          if (scheduling_policy != SCHED_PREEMPT_RR) break; // Check if scheduling policy changed from RR
          
          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            p->state = RUNNING;
            if (p->from_forkp) {
              if (!holding(&tickslock)) {
                acquire(&tickslock);
                xticks = ticks;
                release(&tickslock);
              }
              else xticks = ticks;
              p->runnable_end = xticks;
              batch_waiting_time += (p->runnable_end) - (p->runnable_start);
            }
            c->proc = p;
            swtch(&c->context, &p->context);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
          }
          release(&p->lock);
        }
        break;

      case SCHED_NPREEMPT_SJF: ;
        // Reset the pointer to the next process to be run by the scheduler to 0
        next_proc = 0;
        int min_estimate = -1;

        for(p = proc; p < &proc[NPROC]; p++) {
          if(scheduling_policy != SCHED_NPREEMPT_SJF) break; // Scheduling policy has changed, break out

          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            if(!p->from_forkp) { // This is not a batch process, start the execution of this process immediately
              p->state = RUNNING;
              c->proc = p;
              swtch(&c->context, &p->context);

              // Process is done running for now.
              // It should have changed its p->state before coming back.
              c->proc = 0;
            }
            else  {
              if(min_estimate == -1) { // If this is the first runnable process encountered, update differently
                next_proc = p;
                min_estimate = p->estimate;
              }
              else if(min_estimate > p->estimate) { // Obtain the minimum estimate
                next_proc = p;
                min_estimate = p->estimate;
              }
            }
          }
          release(&p->lock);
        }

        if (!next_proc) continue;

        acquire(&next_proc->lock);
        next_proc->state = RUNNING; // Start the execution of next process

        if (!holding(&tickslock)) {
           acquire(&tickslock);
           xticks = ticks;
           release(&tickslock);
        }
        else xticks = ticks;
        next_proc->cpu_burst_start_tick = xticks;
        next_proc->runnable_end = xticks;
        batch_waiting_time += (next_proc->runnable_end) - (next_proc->runnable_start);

        c->proc = next_proc;
        swtch(&c->context, &next_proc->context); // Context switch to the process
        
        // Process is done running for now
        // Process no longer has RUNNING state
        c->proc = 0;

        release(&next_proc->lock);
        break;

      case SCHED_PREEMPT_UNIX:
        for(p = proc; p < &proc[NPROC]; p++) {
          if(scheduling_policy != SCHED_PREEMPT_UNIX) break; // Scheduling policy has changed, break out

          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            if(p->from_forkp) { // Update unix scheduling variables only if they are from the batch
              p->cpu_usage = (p->cpu_usage >> 1);
              p->dynamic_priority = p->base_priority + (p->cpu_usage >> 1);
              // printf("PID: %d   |     CPU Usage: %d    |    Dynamic Priority: %d\n", p->pid, p->cpu_usage, p->dynamic_priority);
            }
          }
          release(&p->lock);
        }

        next_proc = 0;
        int min_priority = -1;

        for(p = proc; p < &proc[NPROC]; p++) {
          if(scheduling_policy != SCHED_PREEMPT_UNIX) break; // Scheduling policy has changed, break out

          acquire(&p->lock);
          if(p->state == RUNNABLE) {
            if(!p->from_forkp) { // This is not a batch process, start the execution of this process immediately
              p->state = RUNNING;
              c->proc = p;
              swtch(&c->context, &p->context);

              // Process is done running for now.
              // It should have changed its p->state before coming back.
              c->proc = 0;
            }
            else {
              if(min_priority == -1) { // If this is the first runnable process encountered, update differently
                next_proc = p;
                min_priority = p->dynamic_priority;
              }
              else if(min_priority > p->dynamic_priority) { // Obtain the minimum priority process
                next_proc = p;
                min_priority = p->dynamic_priority;
              }
            }
          }
          release(&p->lock);
        }

        if(!next_proc) continue;

        acquire(&next_proc->lock);
        next_proc->state = RUNNING; // Start the execution of next process

        if (!holding(&tickslock)) {
           acquire(&tickslock);
           xticks = ticks;
           release(&tickslock);
        }
        else xticks = ticks;
        next_proc->runnable_end = xticks;
        batch_waiting_time += (next_proc->runnable_end) - (next_proc->runnable_start);
        
        c->proc = next_proc;
        swtch(&c->context, &next_proc->context); // Context switch to the process
        
        // Process is done running for now
        // Process no longer has RUNNING state
        c->proc = 0;

        release(&next_proc->lock);
        break;

      default: break;
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
  acquire(&p->lock);
  p->state = RUNNABLE;

  if(p->from_forkp) { // Perform various scheduling algorithm and batch statistic calculations only if the process is from the batch
    uint xticks;
    if (!holding(&tickslock)) {
       acquire(&tickslock);
       xticks = ticks;
       release(&tickslock);
    }
    else xticks = ticks;
    p->cpu_burst_end_tick = xticks;
    p->runnable_start = xticks;

    int cpu_burst_length = p->cpu_burst_end_tick - p->cpu_burst_start_tick; // Actual CPU burst length
    if (cpu_burst_length) { // If the actual length is non zero, update the estimate

      // ################## Actual CPU Burst Statistics ##################
      cpu_bursts_count++;
      cpu_bursts_avg += cpu_burst_length;
      if(cpu_burst_length > cpu_bursts_max) cpu_bursts_max = cpu_burst_length;
      if(cpu_bursts_min == -1 || cpu_burst_length < cpu_bursts_min) cpu_bursts_min = cpu_burst_length;

      if(p->estimate) { // If the estimate (BEFORE UPDATING) is also non zero, update the error counts
        // ################## Error CPU Burst Statistics ##################
        cpu_bursts_error_count++;
        if(p->estimate > cpu_burst_length) cpu_bursts_error_avg += p->estimate - cpu_burst_length; // Add the absolute error
        else cpu_bursts_error_avg += cpu_burst_length - p->estimate;
      }

      p->estimate = cpu_burst_length - (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM
                    + (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM; // New estimate to predict the next cpu burst
    }

    if(p->estimate){
      // ################## Estimated CPU Burst Statistics (AFTER UPDATING) ##################
      cpu_est_bursts_count++;
      cpu_est_bursts_avg += p->estimate;
      if(p->estimate > cpu_est_bursts_max) cpu_est_bursts_max = p->estimate;
      if(cpu_est_bursts_min == -1 || cpu_est_bursts_min > p->estimate) cpu_est_bursts_min = p->estimate;
    }

    // ############################## UNIX SCHEDULER CALCULATIONS ##############################
    p->cpu_usage += SCHED_PARAM_CPU_USAGE;
  }

  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.

  // ##### OFFICIAL SOLUTION #####
  release(&myproc()->lock);
  acquire(&tickslock);
  myproc()->start_time = ticks;
  release(&tickslock);
  // ##### OFFICIAL SOLUTION #####


  // ##### OUR SOLUTION #####
  // acquire(&tickslock);
  // myproc()->start_time = ticks;
  // release(&tickslock);
  // ##### OUR SOLUTION #####

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

  if(p->from_forkp) { // Only perform scheduling algorithm and batch statistic updates on processes from the batch
    uint xticks;
    if (!holding(&tickslock)) {
       acquire(&tickslock);
       xticks = ticks;
       release(&tickslock);
    }
    else xticks = ticks;
    p->cpu_burst_end_tick = xticks;

    int cpu_burst_length = p->cpu_burst_end_tick - p->cpu_burst_start_tick; // Actual CPU burst length
    if (cpu_burst_length) { // If the actual length is non zero, update the estimate

      // ################## Actual CPU Burst Statistics ##################
      cpu_bursts_count++;
      cpu_bursts_avg += cpu_burst_length;
      if(cpu_burst_length > cpu_bursts_max) cpu_bursts_max = cpu_burst_length;
      if(cpu_bursts_min == -1 || cpu_burst_length < cpu_bursts_min) cpu_bursts_min = cpu_burst_length;

      if(p->estimate) { // If the estimate (BEFORE UPDATING) is also non zero, update the error counts
        // ################## Error CPU Burst Statistics ##################
        cpu_bursts_error_count++;
        if(p->estimate > cpu_burst_length) cpu_bursts_error_avg += p->estimate - cpu_burst_length; // Add the absolute error
        else cpu_bursts_error_avg += cpu_burst_length - p->estimate;
      }

      p->estimate = cpu_burst_length - (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM
                    + (SCHED_PARAM_SJF_A_NUMER*cpu_burst_length)/SCHED_PARAM_SJF_A_DENOM; // New estimate to predict the next cpu burst
    }

    if(p->estimate){
      // ################## Estimated CPU Burst Statistics (AFTER UPDATING) ##################
      cpu_est_bursts_count++;
      cpu_est_bursts_avg += p->estimate;
      if(p->estimate > cpu_est_bursts_max) cpu_est_bursts_max = p->estimate;
      if(cpu_est_bursts_min == -1 || cpu_est_bursts_min > p->estimate) cpu_est_bursts_min = p->estimate;
    }

    // ############################## UNIX SCHEDULER CALCULATIONS ##############################
    p->cpu_usage += (SCHED_PARAM_CPU_USAGE >> 1);
  }

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

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
        if (p->from_forkp) {
          uint xticks;
          if (!holding(&tickslock)) {
            acquire(&tickslock);
            xticks = ticks;
            release(&tickslock);
          }
          else xticks = ticks;
          p->runnable_start = xticks;
        }
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

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
        if (p->from_forkp) {
          uint xticks;
          if (!holding(&tickslock)) {
            acquire(&tickslock);
            xticks = ticks;
            release(&tickslock);
          }
          else xticks = ticks;
          p->runnable_start = xticks;
        }
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


// **************************** Innovations innovated by UG-IITK'24 **************************** /

int
getppid(void){
  // since child is accessing some other process' (parent process') PID, it's lock must be acquired first
  acquire(&wait_lock);
  struct proc *p = myproc()->parent;
  if(p == 0) {
    release(&wait_lock);
    return -1;
  }

  acquire(&(p->lock));
  int ppid = p->pid;
  release(&(p->lock));
  release(&wait_lock);

  return ppid;
}

int
forkf(uint64 f){
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
  np->trapframe->epc = f; // change program counter to execute function f after returning to user mode

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
  np->from_forkp = 0;
  release(&np->lock);

  return pid;
}

int
waitpid(int pid, uint64 addr)
{ // Similar in implementation to wait().

  // ##### OUR SOLUTION #####
  // if (pid < -1 || pid == 0) { // invalid input
  //   return -1;
  // }
  // ##### OUR SOLUTION #####

  struct proc *np;
  struct proc *p = myproc();


  // ##### OFFICIAL SOLUTION #####
  int found = 0;

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
  // ##### OFFICIAL SOLUTION #####

  // ##### OUR SOLUTION #####
  // int havekids, pid;
  // acquire(&wait_lock);
  // for(;;){
  //   // Scan through table looking for exited children.
  //   havekids = 0;
  //   for(np = proc; np < &proc[NPROC]; np++){
  //     if (np->parent == p ){
  //       // make sure the child isn't still in exit() or swtch().
  //       acquire(&np->lock);

  //       if (x != -1 && np->pid != x) { // only proceed if the process is a child with the given pid or if x = -1
  //         release(&np->lock);
  //         continue;
  //       }

  //       havekids = 1;
  //       if(np->state == ZOMBIE){
  //         // Found the one.
  //         pid = np->pid;
  //         if(addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
  //                                 sizeof(np->xstate)) < 0) {
  //           release(&np->lock);
  //           release(&wait_lock);
  //           return -1;
  //         }
  //         freeproc(np);
  //         release(&np->lock);
  //         release(&wait_lock);
  //         return pid;
  //       }
  //       release(&np->lock);
  //     }
  //   }

  //   // No point waiting if we don't have any children.
  //   if (!havekids || p->killed) {
  //     release(&wait_lock);
  //     return -1;
  //   }
    
  //   // Wait for a child to exit.
  //   sleep(p, &wait_lock);  //DOC: wait-sleep
  // } 
}

// ##### OFFICIAL SOLUTION #####
void
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

    printf("pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p", pid, ppid, state, p->name, p->creat_time, p->start_time, (p->end_time == -1) ? xticks-p->start_time : p->end_time-p->start_time, p->sz);
    printf("\n");
  }
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
     pstat.ctime = p->creat_time;
     pstat.stime = p->start_time;
     pstat.etime = (p->end_time == -1) ? xticks-p->start_time : p->end_time-p->start_time;
     pstat.size = p->sz;
     if(copyout(myproc()->pagetable, addr, (char *)&pstat, sizeof(pstat)) < 0) return -1;
     return 0;
  }
  else return -1;
}
// ##### OFFICIAL SOLUTION #####


// ##### OUR SOLUTION #####
// void
// ps(void)
// {
//   static char *states[] = {
//   [UNUSED]    "unused",
//   [SLEEPING]  "sleep ",
//   [RUNNABLE]  "runble",
//   [RUNNING]   "run   ",
//   [ZOMBIE]    "zombie"
//   };
//   struct proc *p;
//   char *state, *name;
//   uint64 sz; // Store all the values in local variables so that p->lock may be released
//   int pid, ppid, creat_time, start_time, etime;

//   printf("\n");
//   for(p = proc; p < &proc[NPROC]; p++){
//     acquire(&p->lock);

//     if (p->state == UNUSED)
//     {
//       release(&p->lock);
//       continue;
//     }
//     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
//       state = states[p->state];
//     else
//       state = "???";

//     if (p->state == ZOMBIE) {
//       etime = p->end_time - p->start_time;
//     } else {
//       acquire(&tickslock);
//       etime = ticks - p->start_time;
//       release(&tickslock);
//     }
//     pid = p->pid; name = p->name; creat_time = p->creat_time; start_time = p->start_time; sz = p->sz;
//     release(&p->lock);

//     acquire(&wait_lock);
//     if (p->parent == 0) {
//       ppid = -1;
//     } else {
//       acquire(&p->parent->lock);
//       ppid = p->parent->pid;
//       release(&p->parent->lock);   
//     }
//     release(&wait_lock);

//     printf("pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%x", 
//             pid, ppid, state, name, creat_time, start_time, etime, sz);
//     printf("\n");
//   }
// }

// int 
// pinfo(int pid, uint64 addr) {
//   if (addr == 0) {
//     return -1;
//   }

//   static char *states[] = {
//   [UNUSED]    "unused",
//   [SLEEPING]  "sleep ",
//   [RUNNABLE]  "runble",
//   [RUNNING]   "run   ",
//   [ZOMBIE]    "zombie"
//   };
//   struct proc *mp = myproc();
//   struct proc *p;
//   struct procstat pstat;

//   if (pid == -1) {
//     p = mp;
//     acquire(&p->lock);
//   } else {
//     for(p = proc; p < &proc[NPROC]; p++){
//       acquire(&p->lock);

//       if (p->pid == pid) {
//         goto found;
//       }

//       release(&p->lock);
//     }
//     return -1;
//   }

// found:
//   pstat.pid = p->pid;
//   pstat.ctime = p->creat_time;
//   pstat.stime = p->start_time;
//   pstat.size = p->sz;

//   int l = 0;
//   if(p->state >= 0 && p->state < NELEM(states) && states[p->state]) {
//     while (states[p->state][l]) { // copy the string
//       pstat.state[l] = states[p->state][l];
//       l++;
//     }
//     pstat.state[l] = '\0';
//   }
//   else {
//     pstat.state[0] = '?';
//     pstat.state[1] = '?';
//     pstat.state[2] = '?';
//     pstat.state[3] = '\0';
//   }

//   l = 0;
//   while (p->name[l]) { // copy the string
//     pstat.command[l] = p->name[l];
//     l++;
//   }
//   pstat.command[l] = '\0';

//   if (p->state == ZOMBIE) {
//     pstat.etime = p->end_time - p->start_time;
//   } else {
//     acquire(&tickslock);
//     pstat.etime = ticks - p->start_time;
//     release(&tickslock);
//   }

//   release(&p->lock);

//   acquire(&wait_lock);
//   if (p->parent) {
//     acquire(&p->parent->lock);
//     pstat.ppid = p->parent->pid;
//     release(&p->parent->lock);
//   } else {
//     pstat.ppid = -1;
//   }
//   release(&wait_lock);

//   if(addr != 0 && copyout(mp->pagetable, addr, (char *) (&pstat),
//                           sizeof(pstat)) < 0) {
//     return -1;
//   }
//   return 0;  
// }
// ##### OUR SOLUTION #####

int schedpolicy(int policy){
  int previous_policy = scheduling_policy;
  scheduling_policy = policy;
  return previous_policy;
}


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
  np->from_forkp = 1;
  np->base_priority = priority;

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


  uint xticks;
  if (!holding(&tickslock)) {
    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
  }
  else xticks = ticks;

  np->runnable_start = xticks; // Used for calculating waiting time

  if (batch_start_time == -1) { // First process from the batch has entered the ready queue
    batch_start_time = xticks;
  }
  batch_size++; // Increment batch size
  batch_size_running++;

  return pid;
}

void 
print_statistics (void) 
{
  printf("\nBatch execution time: %d\nAverage turn-around time: %d\nAverage waiting time: %d\nCompletion time: avg: %d, max: %d, min: %d\n",\
        batch_execution_time, batch_turnaround_time / batch_size, batch_waiting_time / batch_size,\
        batch_completion_time_avg / batch_size, batch_completion_time_max, batch_completion_time_min);
  if(scheduling_policy == SCHED_NPREEMPT_SJF) {
    printf("CPU bursts: count: %d, avg: %d, max: %d, min: %d\nCPU burst estimates: count: %d, avg: %d, max: %d, min: %d\n\
            CPU burst estimation error: count: %d, avg: %d\n",\
            cpu_bursts_count, (cpu_bursts_count ? cpu_bursts_avg / cpu_bursts_count : 0), cpu_bursts_max, cpu_bursts_min,\
            cpu_est_bursts_count, (cpu_est_bursts_count ? cpu_est_bursts_avg / cpu_est_bursts_count : cpu_est_bursts_count),\
            cpu_est_bursts_max, cpu_est_bursts_min, cpu_bursts_error_count, (cpu_bursts_error_count ? cpu_bursts_error_avg / cpu_bursts_error_count : cpu_bursts_error_count));
  }
  
  batch_size = 0;
  batch_size_running = 0;
  batch_start_time = -1;
  batch_end_time = 0;
  batch_execution_time = 0;
  batch_turnaround_time = 0;
  batch_waiting_time = 0;
  batch_completion_time_avg = 0;
  batch_completion_time_min = -1;
  batch_completion_time_max = 0;

  cpu_bursts_count = 0;
  cpu_bursts_avg = 0;
  cpu_bursts_max = 0;
  cpu_bursts_min = -1;

  cpu_est_bursts_count = 0;
  cpu_est_bursts_avg = 0;
  cpu_est_bursts_max = 0;
  cpu_est_bursts_min = -1;

  cpu_bursts_error_count = 0;
  cpu_bursts_error_avg = 0;
}