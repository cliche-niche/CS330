#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
  int i, y;
  int x = fork();

  for (i=0; i<5; i++) {
     fprintf(2, "%d: looped %d times", getpid(), i);
     yield();
  }
  if (x > 0) {
     printf("%d: before wait\n", getpid());
     y = wait(0);
     printf("%d: after wait for pid %d\n", getpid(), y);
  }
  exit(0);
}
