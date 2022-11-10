#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int m, n, x;

  if (argc != 3) {
     fprintf(2, "syntax: testGetPwaitP m n\nAborting...\n");
     exit(0);
  }

  m = atoi(argv[1]);
  if (m <= 0) {
     fprintf(2, "Invalid input\nAborting...\n");
     exit(0);
  }
  n = atoi(argv[2]);
  if ((n != 0) && (n != 1)) {
     fprintf(2, "Invalid input\nAborting...\n");
     exit(0);
  }

  x = fork();
  if (x < 0) {
     fprintf(2, "Error: cannot fork\nAborting...\n");
     exit(0);
  }
  else if (x > 0) {
     if (n) sleep(m);
     fprintf(1, "%d: Parent.\n", getpid());
     fprintf(1, "Return value of waitpid=%d\n", waitpid(x, 0));
  }
  else {
     if (!n) sleep(m);
     fprintf(1, "%d: Child with parent %d.\n", getpid(), getppid());
  }

  exit(0);
}
