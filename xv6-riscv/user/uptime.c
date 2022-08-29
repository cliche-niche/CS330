#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void){
	printf("The uptime is: %l\n", uptime());
	exit(0);
}