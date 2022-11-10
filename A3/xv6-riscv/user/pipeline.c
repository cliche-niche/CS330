#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pipefd[2], x, y, n, i;

  if (argc != 3) {
     fprintf(2, "syntax: pipeline n x\nAborting...\n");
     exit(0);
  }
  n = atoi(argv[1]);
  if (n <= 0) {
     fprintf(2, "Invalid input\nAborting...\n");
     exit(0);
  }
  x = atoi(argv[2]);

  x += getpid();
  fprintf(1, "%d: %d\n", getpid(), x);

  for (i=2; i<=n; i++) {
     if (pipe(pipefd) < 0) {
        fprintf(2, "Error: cannot create pipe\nAborting...\n");
        exit(0);
     }
     if (write(pipefd[1], &x, sizeof(int)) < 0) {
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
        if (read(pipefd[0], &x, sizeof(int)) < 0) {
           fprintf(2, "Error: cannot read from pipe\nAborting...\n");
           exit(0);
        }
	x += getpid();
	fprintf(1, "%d: %d\n", getpid(), x);
	close(pipefd[0]);
     }
  }

  exit(0);
}
