#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


// ! Remember to delete this file both from the directory as well as from makefile
int main(){
    int b=0, x=69;
    printf("b virtual address : %x\n", &b);
    printf("x virtual address : %x\n", &x);
    printf("b actual physical address : %x\n", getpa(&b));
    printf("x actual physical address : %x\n", getpa(&x));
    exit(0);
}