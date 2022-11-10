#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i, j, n, r, barrier_id1, barrier_id2;

  if (argc != 3) {
     fprintf(2, "syntax: barriergrouptest numprocs numrounds\nAborting...\n");
     exit(0);
  }

  n = atoi(argv[1]);
  r = atoi(argv[2]);
  barrier_id1 = barrier_alloc();
  barrier_id2 = barrier_alloc();
  fprintf(1, "%d: got barrier array ids %d, %d\n\n", getpid(), barrier_id1, barrier_id2);

  for (i=0; i<n-1; i++) {
     if (fork() == 0) {
	if ((i%2) == 0) {
           for (j=0; j<r; j++) {
	      barrier(j, barrier_id1, n/2);
	   }
        }
	else {
	   for (j=0; j<r; j++) {
              barrier(j, barrier_id2, n/2);
           }
        }
	exit(0);
     }
  }
  for (j=0; j<r; j++) {
     barrier(j, barrier_id2, n/2);
  }
  for (i=0; i<n-1; i++) wait(0);
  barrier_free(barrier_id1);
  barrier_free(barrier_id2);
  exit(0);
}
