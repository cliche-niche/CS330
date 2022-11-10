#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void){
	printf("The uptime is: %d\n", uptime());
	exit(0);
}