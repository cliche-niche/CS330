#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/procstat.h"
#include "user/user.h"

// all testing functions will be of the form test_*
int g (int x)
{
   return x*x;
}

int f (void)
{
   int x = 10;

   fprintf(1, "Hello world! %d\n", g(x));
   return 0;
}

void test_getppid() {
    int b = fork();
    if(b == 0) {
        printf("My pid %d, Parent's pid %d\n", getpid(), getppid());
        exit(0); // kill child to not interfere with parent's aspirations
    }
    else {
        wait(0);
        printf("My pid %d, Child's pid %d\n", getpid(), b);
    }
}

void test_yield() {
    printf("Yielded?\n");
    yield();
    printf("Yes\n");
}

void test_getpa() {
    int b=0, x=69;
    printf("b virtual address : %x\n", &b);
    printf("x virtual address : %x\n", &x);
    printf("b actual physical address : %x\n", getpa(&b));
    printf("x actual physical address : %x\n", getpa(&x));
    printf("test_yield actual physical address : %x\n", getpa(&test_yield));
}

void test_forkf() {
    int x = forkf(f);
    if (x < 0) {
        fprintf(2, "Error: cannot fork\nAborting...\n");
        exit(0);
    }
    else if (x > 0) {
        sleep(5);
        fprintf(1, "%d: Parent.\n", getpid());
        wait(0);
    }
    else {
        fprintf(1, "%d: Child.\n", getpid());
    }
}

void test_waitpid(){
    int b = fork();
    if(b==0){
        printf("This is child\n");
    }else{
        int k = waitpid(b,0);
        if(k==-1) printf("Failed\n");
        else printf("Succesful wait_call. Returned %d\n",k);
        b=fork();
        if(b==0){
            printf("This is child\n");
        }else{
            int k = waitpid(-1,0);
            if(k==-1) printf("Failed\n");
            else printf("Succesful wait_call. Returned %d\n",k);
            b=fork();
            if(b==0){
                printf("This is child\n");
            }else{
                int k = waitpid(b+1,0);
                if(k==-1){
                    waitpid(-1,0);
                    printf("Failed\n");
                }
                else printf("Succesful wait_call. Returned %d\n",k);
            }
        }
    }
}

void test_ps(){
    ps();
}

void test_pinfo() {
    struct procstat pstat;

    int x = fork();
    if (x < 0) {
        fprintf(2, "Error: cannot fork\nAborting...\n");
        exit(0);
    }
    else if (x > 0) {
        sleep(1);
        fprintf(1, "%d: Parent.\n", getpid());
        if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
        if (pinfo(x, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
        fprintf(1, "Return value of waitpid=%d\n", waitpid(x, 0));
    }
    else {
        fprintf(1, "%d: Child.\n", getpid());
        if (pinfo(-1, &pstat) < 0) fprintf(1, "Cannot get pinfo\n");
        else fprintf(1, "pid=%d, ppid=%d, state=%s, cmd=%s, ctime=%d, stime=%d, etime=%d, size=%p\n\n",
            pstat.pid, pstat.ppid, pstat.state, pstat.command, pstat.ctime, pstat.stime, pstat.etime, pstat.size);
    }
}

// ! Remember to delete this file both from the directory as well as from makefile
int main(){
    test_pinfo();
    test_ps();
    test_forkf();
    test_getppid();
    test_yield();
    test_getpa();
    test_waitpid();

    exit(0);
}