// ########################### implementation by UG - IITK 24 ########################### //
struct cond_buffer{
    int x;
    int full;
    struct sleeplock lk;
    cond_t inserted;
    cond_t deleted;
};

typedef int sem_buffer;
// ########################### UG IITK - 24 