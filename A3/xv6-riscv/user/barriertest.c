#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int i, j, n, r, barrier_id;

  if (argc != 3) {
     fprintf(2, "syntax: barriertest numprocs numrounds\nAborting...\n");
     exit(0);
  }

  n = atoi(argv[1]);
  r = atoi(argv[2]);
  barrier_id = barrier_alloc();
  fprintf(1, "%d: got barrier array id %d\n\n", getpid(), barrier_id);

  for (i=0; i<n-1; i++) {
     if (fork() == 0) {
        for (j=0; j<r; j++) {
	   barrier(j, barrier_id, n);
	}
	exit(0);
     }
  }
  for (j=0; j<r; j++) {
     barrier(j, barrier_id, n);
  }
  for (i=0; i<n-1; i++) wait(0);
  barrier_free(barrier_id);
  exit(0);
}
