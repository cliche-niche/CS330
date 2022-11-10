#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pipefd1[2], pipefd2[2], x;
  char y = '0';

  if (pipe(pipefd1) < 0) {
     fprintf(2, "Error: cannot create pipe\nAborting...\n");
     exit(0);
  }

  if (pipe(pipefd2) < 0) {
     fprintf(2, "Error: cannot create pipe\nAborting...\n");
     exit(0);
  }

  x = fork();
  if (x < 0) {
     fprintf(2, "Error: cannot fork\nAborting...\n");
     exit(0);
  }
  else if (x > 0) {
     if (write(pipefd1[1], &y, 1) < 0) {
        fprintf(2, "Error: cannot write to pipe\nAborting...\n");
	exit(0);
     }
     if (read(pipefd2[0], &y, 1) < 0) {
        fprintf(2, "Error: cannot read from pipe\nAborting...\n");
        exit(0);
     }
     fprintf(1, "%d: received pong\n", getpid());
     close(pipefd1[0]);
     close(pipefd1[1]);
     close(pipefd2[0]);
     close(pipefd2[1]);
     wait(0);
  }
  else {
     if (read(pipefd1[0], &y, 1) < 0) {
        fprintf(2, "Error: cannot read from pipe\nAborting...\n");
        exit(0);
     }
     fprintf(1, "%d: received ping\n", getpid());
     if (write(pipefd2[1], &y, 1) < 0) {
        fprintf(2, "Error: cannot write to pipe\nAborting...\n");
        exit(0);
     }
     close(pipefd1[0]);
     close(pipefd1[1]);
     close(pipefd2[0]);
     close(pipefd2[1]);
  }

  exit(0);
}
