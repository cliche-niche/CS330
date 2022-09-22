#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

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

// all testing functions will be of the form test_*
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
    printf("f address = %x\n", f);
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

void test_func_pointer(void (*f)()){
    f();
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

// ! Remember to delete this file both from the directory as well as from makefile
int main(){
    // test_forkf();
    // test_func_pointer((void*) f);
    // test_forkf((void*) &test_getppid);
    // test_getppid();
    // test_yield();
    // test_getpa();
    test_waitpid();
    exit(0);
}