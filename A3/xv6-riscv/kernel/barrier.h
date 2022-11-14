//implementation by UG - IITK 24 //
struct barrier{
    int count;
    struct sleeplock lk;
    cond_t cv;
    int pid;
};

//UG IITK - 24 