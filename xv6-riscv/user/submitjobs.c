#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char buf[128], prio[4], policy[2];
  char **args;
  int i, j, k;

  args = (char**)malloc(sizeof(char*)*16);
  for (i=0; i<16; i++) args[i] = 0;

  gets(buf, sizeof(buf));
  policy[0] = buf[0];
  policy[1] = '\0';
  schedpolicy(atoi((const char*)policy));
  while (1) {
     gets(buf, sizeof(buf));
     if(buf[0] == 0) break;
     i=0;
     while (buf[i] != ' ') {
        prio[i] = buf[i];
	i++;
     }
     prio[i] = '\0';
     k=0;
     while (1) {
	i++;
        j=0;
	if (!args[k]) args[k] = (char*)malloc(sizeof(char)*32);
        while ((buf[i] != ' ') && (buf[i] != '\n')) {
           args[k][j] = buf[i];
           i++;
	   j++;
        }
        args[k][j] = '\0';
	if (buf[i] == '\n') {
	   break;
	}
	k++;
     }
     if (forkp(atoi((const char*)prio)) == 0) exec(args[0], args);
  }

  exit(0);
}
