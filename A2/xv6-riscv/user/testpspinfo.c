#include "kernel/types.h"
#include "kernel/procstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int m, n, x;
  struct procstat pstat;

  if (argc != 3) {
     fprintf(2, "syntax: testpspinfo m n\nAborting...\n");
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
     ps();
     fprintf(1, "\n");
     if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
     else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n", pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
     if (pinfo(x, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
     else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n", pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
     fprintf(1, "Return value of waitpid=%d\n", waitpid(x, 0));
  }
  else {
     if (!n) sleep(m);
     fprintf(1, "%d: Child.\n", getpid());
     sleep(1);
     if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
     else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n", pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
  }

  exit(0);
}
