#include "kernel/types.h"
#include "user/user.h"

#define OUTER_BOUND 5
#define INNER_BOUND 5000000
#define SIZE 100

int
main(int argc, char *argv[])
{
    int array[SIZE], i, j, k, sum=0, pid=getpid();
    unsigned start_time, end_time;

    start_time = uptime();
    for (k=0; k<OUTER_BOUND; k++) {
       for (j=0; j<INNER_BOUND; j++) for (i=0; i<SIZE; i++) sum += array[i];
       fprintf(1, "%d", pid);
       yield();
    }
    end_time = uptime();
    printf("\nTotal sum: %d\n", sum);
    printf("Start time: %d, End time: %d, Total time: %d\n", start_time, end_time, end_time-start_time);
    exit(0);
}
