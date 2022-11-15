//implementation by UG - IITK 24 //
struct barrier{
    int count;
    struct sleeplock lk;
    cond_t cv;
    int pid;
};

#define NUM_BARRIERS 10

//UG IITK - 24 