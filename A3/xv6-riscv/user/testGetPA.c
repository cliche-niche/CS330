#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int x = 3;
  int y = 4;
  int *a = (int*)malloc(1025*sizeof(int));
  fprintf(1, "x=%d, &x=%p, pa=%p\n", x, &x, getpa(&x));
  fprintf(1, "y=%d, &y=%p, pa=%p\n", y, &y, getpa(&y));
  fprintf(1, "a[0]=%d, &a[0]=%p, pa=%p\n", a[0], &a[0], getpa(&a[0]));
  fprintf(1, "a[1024]=%d, &a[1024]=%p, pa=%p\n", a[1024], &a[1024], getpa(&a[1024]));

  exit(0);
}
