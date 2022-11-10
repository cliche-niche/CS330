#include "kernel/types.h"
#include "user/user.h"

void primes (int rfd, int primecount)
{
   int x, y, z, pipefd[2];
   int count = 0;

   if (pipe(pipefd) < 0) {
      fprintf(2, "Error: cannot create pipe\nAborting...\n");
      exit(0);
   }

   if (read(rfd, &x, sizeof(int)) <= 0) {
      fprintf(2, "Error: cannot read from pipe\nAborting...\n");
      exit(0);
   }
   primecount++;
   fprintf(1, "%d: prime %d\n", primecount, x);
   while (read(rfd, &y, sizeof(int)) > 0) {
      if ((y % x) != 0) {
         if (write(pipefd[1], &y, sizeof(int)) <= 0) {
            fprintf(2, "Error: cannot write to pipe\nAborting...\n");
            exit(0);
         }
	 count++;
      }
   }
   close(rfd);
   close(pipefd[1]);
   if (count) {
      z = fork();
      if (z < 0) {
         fprintf(2, "Error: cannot fork\nAborting...\n");
         exit(0);
      }
      else if (z > 0) {
	 close(pipefd[0]);
         wait(0);
      }
      else primes(pipefd[0], primecount);
   }
   else close(pipefd[0]);
   exit(0);
}
     
int
main(int argc, char *argv[])
{
  int pipefd[2], x, y, i, count=0, primecount=1;

  if (argc != 2) {
     fprintf(2, "syntax: primes n\nAborting...\n");
     exit(0);
  }
  y = atoi(argv[1]);
  if (y < 2) {
     fprintf(2, "Invalid input\nAborting...\n");
     exit(0);
  }

  if (pipe(pipefd) < 0) {
     fprintf(2, "Error: cannot create pipe\nAborting...\n");
     exit(0);
  }

  fprintf(1, "1: prime 2\n");
  for (i=3; i<=y; i++) {
     if ((i%2) != 0) {
        if (write(pipefd[1], &i, sizeof(int)) <= 0) {
           fprintf(2, "Error: cannot write to pipe\nAborting...\n");
           exit(0);
        }
	count++;
     }
  }
  close(pipefd[1]);
  if (count) {
     x = fork();
     if (x < 0) {
        fprintf(2, "Error: cannot fork\nAborting...\n");
        exit(0);
     }
     else if (x > 0) {
	close(pipefd[0]);
        wait(0);
     }
     else primes(pipefd[0], primecount);
  }
  else close(pipefd[0]);

  exit(0);
}
