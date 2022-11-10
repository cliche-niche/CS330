//implementation by UG - IITK 24 //
#include "types.h"
#include "sleeplock.h"
#include "condvar.h"


struct semaphore{
    int val;
    struct sleeplock lk;
    cond_t* cv;
    //for debugging
};

//UG IITK - 24 