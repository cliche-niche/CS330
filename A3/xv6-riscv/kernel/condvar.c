//implemented by UG - IITK 24

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "condvar.h"

void
cond_wait(cond_t *cv, struct sleeplock *lock){
    if(lock->locked == 1){
        condsleep(cv, lock);
    }
}

void
cond_broadcast (cond_t *cv){
    printf("BROADCASTING!\n");
    wakeup((void*) cv);
}

void
cond_signal (cond_t *cv){
    wakeupone(&cv);
}

// UG IITK - 24 //