#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int primes[]={2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,59,61,67,71,73,79,83,89,97}; // global array storing the primes from 2 to 100 (both inclusive)

void prime_factorize (int n, int i) {   // recursive function which attempts to divide n by the i'th prime
    if (n == 1 || i >= sizeof(primes) / sizeof(primes[0]) ) { // base case
        return;
    }
    
    // Open pipe for communication between parent (writes) and child (reads)
    int pipefd[2];
    
    if (pipe(pipefd) < 0) {
        fprintf(2, "error while creating pipe.\n");
        exit(1);
    }
    
    int b = fork();
    
    if (b < 0) {
        fprintf(2, "error while forking.\n");
        close(pipefd[0]);   // closing the pipe in case of any error
        close(pipefd[1]);
        exit(1);
    }
    else if (b > 0) {   // parent
        close(pipefd[0]);   // parent need not read
        
        if (!(n % primes[i])) {
            while (!(n % primes[i])) {
                printf("%d, ", primes[i]);  // printing primes with multiplicity
                n /= primes[i];
            }
            printf("[%d]\n", getpid());
        }
        
        if (write(pipefd[1], &n, sizeof(int)) < 0) {    // delegating the remaining n to child
            fprintf(2, "error while writing to child.\n");
            exit(1);
        }
        close(pipefd[1]);
        wait(0);
    }
    else {  // child
        close(pipefd[1]);   // child need not write
        
        if (read(pipefd[0], &n, sizeof(int)) < 0) {     // reading from parent
            fprintf(2, "error while reading from parent.\n");
            exit(1);
        }
        
        close(pipefd[0]);
        prime_factorize(n, i+1);    // recurring for the next prime
    }
    
    return;
}


int main(int argc, char* argv[]){

	if (argc != 2) {
		fprintf(2, "error: expected 1 argument but received %d.\n", argc-1);
		exit(1);
	}

    int n = 0;
    for (int i = 0; ; i++) {
        if (argv[1][i] == '\0') break;
        if (argv[1][i] < '0' || argv[1][i] > '9') {
            fprintf(2, "Please enter appropriate value of n\n");
            exit(1);
        }
        
        n = n * 10 + argv[1][i] - '0';
    }

	if (n < 2 || n > 100) {
		fprintf(2, "error: n (%d) should be a positive integer between 2 and 100 (both inclusive).\n", n);
		exit(1);
	}

	prime_factorize(n, 0);  // calling the recursive function for the first prime

	exit(0);
}
