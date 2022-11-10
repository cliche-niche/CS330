#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  fprintf(1, "%l ticks\n", uptime());

  exit(0);
}
