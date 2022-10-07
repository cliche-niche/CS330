#include "kernel/types.h"
#include "user/user.h"

int g (int x)
{
   return x*x;
}

int f (void)
{
   int x = 10;

   fprintf(2, "Hello world! %d\n", g(x));
   return 0;
}

int
main(int argc, char *argv[])
{
  int m, n, x;

  if (argc != 3) {
     fprintf(2, "syntax: testForkfSleep m n\nAborting...\n");
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

  x = forkf(f);
  if (x < 0) {
     fprintf(2, "Error: cannot fork\nAborting...\n");
     exit(0);
  }
  else if (x > 0) {
     if (n) sleep(m);
     fprintf(1, "%d: Parent.\n", getpid());
     wait(0);
  }
  else {
     if (!n) sleep(m);
     fprintf(1, "%d: Child.\n", getpid());
  }

  exit(0);
}
