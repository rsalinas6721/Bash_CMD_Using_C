#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

// Global variable is used to identify whether a process can be run in background
int stopped = 1;

/*-------------------------------
Function printExitStatus is declared
This function displays current exit status
-------------------------------*/
void actionSigstop(int signo){
    if (stopped == 0){
        char* output = "Exiting foreground-only mode\n";
        write(1,output, 29);
        stopped = 1;
    }
    else if (stopped == 1) {
	char* output = "Entering foreground-only mode\n";
	write(1, output, 30);
	stopped = 0;
    }

}

/*-------------------------------
Main is declared: Program Starts
-------------------------------*/
int main() {

/*-------------------------------
Sigaction strucs are declared to ignore ctrl+c and catch ctrl+z
-------------------------------*/
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = actionSigstop;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    struct sigaction ignore = {0};
    ignore.sa_handler = SIG_IGN;
    sigfillset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    sigaction(SIGINT, &ignore, NULL);

    int pid = getpid(); 		// PID is retrieved and initialized
    int exitStatus = 0;			// Used for status sunction to return status
    int background = 0;			// Used to identify whether it's a background process
    char inputFile[512] = ""; 	// Used to hold name of input redirection file
    char outputFile[512] = ""; 	// Used to hold name of output redirection file
    char* userInput[512];		// Used to hold user input commands
    char inputCommand[2048]; 	// Used to hold user input
    int looping = 1;			// Keeps program running until user exits


// User input is cleared and set to NULL after each itteration for new command.
	while(looping) {
        int y;
        for (y=0; userInput[y] != NULL; y++) {
            userInput[y] = NULL;
        }
		inputCommand[0] = '\0';	// User input is cleared
		background = 0;			// Background identifier is cleared
		inputFile[0] = '\0';	// Input file name is cleared
		outputFile[0] = '\0';	// Output file name is clered
    	printf(": ");
    	fflush(stdout);
    	fgets(inputCommand, 2048, stdin);				// User input is retrieved, stored in inputCommand
        inputCommand[strcspn(inputCommand,"\n")] = 0;	// New line character is removed from inputCommand

// No input is checked.
// If usserinput is empty, program continues to rerun while loop.
		if (!strcmp(inputCommand, "")) {
            continue;
        }

// Program retrieves input and searches for characters &, <, >, and &&
// Program uses strtok to seperate input based on a " " space.
    	const char space[2] = " ";
    	char *token = strtok(inputCommand, space);
        int i;
    	for (i=0; token; i++) {					// If a & is seen, background int is set to 1
    		if (strcmp(token, "&") == 0) {
    			background = 1;
    		}
    		else if (strcmp(token, "<")== 0) {	// If a < is seen, the next token is set to inputFile
    			token = strtok(NULL, " ");
    			strcpy(inputFile, token);
    		}
    		else if (strcmp(token, ">")== 0) {	// If a > is seen, the next token is set to outputFile
    			token = strtok(NULL, " ");
    			strcpy(outputFile, token);
    		}
		else if (strcmp(token, "$$")== 0) {	// If a $$ is seen, pid is printed to screen
				token = strtok(NULL, " ");
				printf("%d\n", pid);
		}
    		else {								// If no special chars are found, the input is part of command.
    			userInput[i] = strdup(token);
    		}
    		token = strtok(NULL, " ");
    	}

// Program checks for empty input, continues if found. Program loops again.
        if (userInput[0] == "") {
            continue;
        }

// If exit is set to input, looping int is set to 0. While loop ends.
        else if (strcmp(userInput[0], "exit") == 0) {
			looping = 0;
		}
// If cd is set to input, runs cd command and changes directories if no error is found.
	else if (strcmp(userInput[0], "cd") == 0) {
		if (userInput[1]) {
			if (chdir(userInput[1]) == -1) {
				printf("Directory not found.\n");
			}
		}
		else {
			chdir(getenv("HOME"));
		}
	}
// Program checks for # as input to find comment. Program loops again.
        else if (userInput[0][0] == '#') {
            continue;
        }
// Program checks for status, prints exitstatus
// Displays current exit status
	else if (strcmp(userInput[0], "status") == 0) {
		if (WIFEXITED(exitStatus)) {
			printf("Exit Value %d\n", WEXITSTATUS(exitStatus));
		}
		else {
			printf("Terminated By Signal %d\n", WTERMSIG(exitStatus));
		}
	}
	else {

// Else run command. This forks and runs command as a child process.
            int in, out;				// In, Out variables are set
            pid_t spawnPid = -5;		// spawnPid is set to 5
            spawnPid = fork();			// Process is forked. Child is Birthed
            if (spawnPid == - 1){		// Program checks for error
                    perror("Error With Fork\n");
                    exit(1);
                    break;
            }
            if (spawnPid == 0){							// If child process is found
				ignore.sa_handler = SIG_DFL;			// ctrl+c is set to default instead of ignored for child process
				sigaction(SIGINT, &ignore, NULL);

                if (strcmp(inputFile, "")) {		// Check if inputFile contents exists
                    in = open(inputFile, O_RDONLY);	// Inputfile is opened
                    if (in == -1) {					// Program checks for errors
                        perror("Error Opening File\n");
                        exit(1);
                	}
                    if (dup2(in, 0) == -1) {		// dup function is called and checked for error
                        perror("Error With DUP2 Function\n");
                        exit(1);
                    }
                    close(in);						// file is closed
            	}
                if (strcmp(outputFile, "")) {		// Check if outputFile contents exists
                    out = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);	// outputFile is opened
                    if (out == -1) {				// Program checks for errors
                        perror("Error Opening File\n");
                        exit(1);
                    }
                    if (dup2(out, 1) == -1) {		// dup function is called and checked for error
                        perror("Error With DUP2 Function\n");
                        exit(1);
                    }
                    close(out);
                }

    	        if (execvp(userInput[0], (char* const*)userInput)) {	// Command is run, Error is checked
                    printf("Error With %s Command\n", userInput[0]);
			int y;
			for(y = 0; y < 512; y++){						// userInput is cleared and userInput is set to NULL
                        userInput[y] = NULL;
                    }
                    exit(1);
                }
            }

            else{ // Program checks to see if the ctrl+z signal is stopped and if command is a background process
                if (background == 1 && stopped == 1){	// Variables are checked
                    pid_t actualPid = waitpid(spawnPid, &exitStatus, WNOHANG);	// Command is run in background. Parent does not wait for child to be completed.
                    printf("The Background PID Is %d\n", spawnPid);					// childPid is printed to screen
                }
                else {
                    pid_t actualPid = waitpid(spawnPid, &exitStatus, 0);	// Command is not a background process.
                }
				// Program displays when a child process running in the background is complete. Prints Exit Process
                while ((spawnPid = spawnPid = waitpid(-1, &exitStatus, WNOHANG)) > 0) {
    			if (WIFEXITED(exitStatus)) {
				printf("Child Process %d Is Complete With Exit Value %d\n", spawnPid, WEXITSTATUS(exitStatus));
			}
			else {
				printf("Child Process %d Is Complete. Terminated By Signal %d\n", spawnPid, WTERMSIG(exitStatus));
			}
                }
            }
	}

// Variables are cleared
        inputCommand[0] = '\0';	// UserInput is cleared
        background = 0;			// Background int is cleared and set to 0
        inputFile[0] = '\0';	// Input file name is cleared
        outputFile[0] = '\0';	//	Output file name is cleared
		int x;
        for(x = 0; x < 512; x++){	// Userinput is set to NULL
            userInput[x] = NULL;
        }
    }
	return 0;
}
