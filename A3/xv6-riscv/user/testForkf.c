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
main(void)
{
  int x = forkf(f);
  if (x < 0) {
     fprintf(2, "Error: cannot fork\nAborting...\n");
     exit(0);
  }
  else if (x > 0) {
     sleep(1);
     fprintf(1, "%d: Parent.\n", getpid());
     wait(0);
  }
  else {
     fprintf(1, "%d: Child.\n", getpid());
  }

  exit(0);
}
