#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    if (argc != 3) {
        fprintf(2, "Please enter both m and n\n");
        exit(1);
    }

    if ( argv[2][0] == '\0' || argv[2][1] != '\0' || (argv[2][0] != '1' && argv[2][0] != '0') ) {
        fprintf(2, "Please enter appropriate value of n\n");
        exit(1);
    }

    int m = 0;
    for (int i = 0; ; i++) {
        if (argv[1][i] == '\0') break;
        
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            fprintf(2, "Please enter appropriate value of m\n");
            exit(1);
        }

        m = m * 10 + argv[1][i] - '0';
    }

    int f = fork();

    if (f < 0) {
        fprintf(2, "Cannot fork\n");
        exit(1);
    }
    else if (f == 0) {
        if (argv[2][0] == '0') {
            sleep(m);
        }
        printf("%d : Child.\n", getpid());
    }
    else {
        if (argv[2][0] == '1') {
            sleep(m);
        }
        printf("%d : Parent.\n", getpid());
        wait(0);
    }
    exit(0);
}