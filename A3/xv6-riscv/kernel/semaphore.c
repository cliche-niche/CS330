//implemented by UG IITK 24 //
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "condvar.h"
#include "semaphore.h"

void
sem_init(struct semaphore *s, int x){
    s->val = x;
    initsleeplock(&s->lk, "sleeplock"); 
}

void
sem_wait (struct semaphore *s){
    acquiresleep(&s->lk);
    while(s->val == 0) cond_wait(&s->cv, &s->lk);
    s->val--;
    releasesleep(&s->lk);
}

void
sem_post (struct semaphore* s){
    acquiresleep(&s->lk);
    s->val++;
    cond_signal(&s->cv);
    releasesleep(&s->lk);
}
//UG IITK 24//