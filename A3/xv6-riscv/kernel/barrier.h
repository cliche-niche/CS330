//implementation by UG - IITK 24 //
#include "types.h"
#include "sleeplock.h"
#include "condvar.h"

struct barrier{
    int count;
    struct sleeplock lk;
    cond_t cv;
    int pid;
};

//UG IITK - 24 