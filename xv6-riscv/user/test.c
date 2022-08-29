#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(){
    if(fork()==0) printf("Child: My pid = %d, and parent pid = %d\n",getpid(),getppid());
    else{
        wait(0);
        printf("Parent: My pid = %d, and parent pid = %d\n",getpid(),getppid());
    }
    exit(0);
}