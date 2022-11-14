// #################### implementation by UG - IITK 24 #################### //
#include "types.h"
#include "sleeplock.h"
#include "condvar.h"


typedef struct semaphore{
    int val;
    struct sleeplock lk;
    cond_t* cv;
    //for debugging
} semaphore;

// #################### UG IITK - 24  ####################