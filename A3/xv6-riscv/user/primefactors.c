#include "kernel/types.h"
#include "user/user.h"

int primes[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97};

int
main(int argc, char *argv[])
{
  int pipefd[2], x, y, n, i=0;

  if (argc != 2) {
     fprintf(2, "syntax: factors n\nAborting...\n");
     exit(0);
  }
  n = atoi(argv[1]);
  if ((n <= 1) || (n > 100)) {
     fprintf(2, "Invalid input\nAborting...\n");
     exit(0);
  }

  while (1) {
     x=0;
     while ((n % primes[i]) == 0) {
        n = n / primes[i];
	fprintf(1, "%d, ", primes[i]);
	x=1;
     }
     if (x) fprintf(1, "[%d]\n", getpid());
     if (n == 1) break;
     i++;
     if (pipe(pipefd) < 0) {
        fprintf(2, "Error: cannot create pipe\nAborting...\n");
        exit(0);
     }
     if (write(pipefd[1], &n, sizeof(int)) < 0) {
        fprintf(2, "Error: cannot write to pipe\nAborting...\n");
        exit(0);
     }
     close(pipefd[1]);
     y = fork();
     if (y < 0) {
        fprintf(2, "Error: cannot fork\nAborting...\n");
	exit(0);
     }
     else if (y > 0) {
	close(pipefd[0]);
        wait(0);
	break;
     }
     else {
        if (read(pipefd[0], &n, sizeof(int)) < 0) {
           fprintf(2, "Error: cannot read from pipe\nAborting...\n");
           exit(0);
        }
	close(pipefd[0]);
     }
  }

  exit(0);
}
