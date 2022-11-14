// ########################### implementation by UG - IITK 24 ########################### //
struct buffer{
    int x;
    int full;
    struct sleeplock lk;
    cond_t inserted;
    cond_t deleted;
};

// ########################### UG IITK - 24 