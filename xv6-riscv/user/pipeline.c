#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){

	if (argc != 3) {
		fprintf(2, "error: expected 2 arguments but received %d.\n", argc-1);
		exit(1);
	}

	int n = 0, x = 0, flag = 0;
    for (int i = 0; ; i++) {
        if (argv[1][i] == '\0') break;
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            fprintf(2, "Please enter appropriate value of n\n");
            exit(1);
        }
        
        n = n * 10 + argv[1][i] - '0';
    }

    for(int i = 0; ; i++){
        if (argv[2][i] == '\0') break;
        if (i == 0 && argv[2][i] == '-'){
            flag = 1;
            continue;
        }
        if(argv[2][i] < '0' || argv[2][i] > '9'){
            fprintf(2, "Please enter appropriate value of x\n");
            exit(1);
        }

        x = x * 10 + argv[2][i] - '0';
    }
    if (flag) x *= -1;

	while (n--) {

		int pipefd[2];

		if (pipe(pipefd) < 0) {
			fprintf(2, "error while creating pipe.\n");
			exit(1);
		}

		int b = fork();

		if (b < 0) {
			fprintf(2, "forking error.\n");
			close(pipefd[0]);   // closing the pipe in case of any error
			close(pipefd[1]);
			exit(1);
		}
		else if (b > 0) {
			close(pipefd[0]);   // parent need not read
			x += getpid();
			printf("%d: %d\n", getpid(), x);
			if (write(pipefd[1], &x, sizeof(int)) < 0) {	// Write the value of x for child
				fprintf(2, "error while writing x.\n");
				exit(1);
			}
			close(pipefd[1]);

			wait(0);	// Waits for the child to complete
			break;
		}
		else if (b == 0) {
			close(pipefd[1]);   // child need not write
			
			if (read(pipefd[0], &x, sizeof(int)) < 0){		// Read the value of x from parent
				fprintf(2, "error while reading x.\n");
				exit(1);
			}
			close(pipefd[0]);
		}
	}
	exit(0);
}
