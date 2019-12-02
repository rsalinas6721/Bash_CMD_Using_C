#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>


int stopped = 1;


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

int main() {
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


	while(looping) {
        	int y;
        	for (y=0; userInput[y] != NULL; y++) {
            		userInput[y] = NULL;
        	}
		inputCommand[0] = '\0';	
		background = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
    		printf(": ");
    		fflush(stdout);
    		fgets(inputCommand, 2048, stdin);
        	inputCommand[strcspn(inputCommand,"\n")] = 0;	


		if (!strcmp(inputCommand, "")) {
            		continue;
        	}


    		const char space[2] = " ";
    		char *token = strtok(inputCommand, space);
       		int i;
    		for (i=0; token; i++) {	
    			if (strcmp(token, "&") == 0) {
    				background = 1;
    			}
    			else if (strcmp(token, "<")== 0) {
    				token = strtok(NULL, " ");
    				strcpy(inputFile, token);
    			}
    			else if (strcmp(token, ">")== 0) {
    				token = strtok(NULL, " ");
    				strcpy(outputFile, token);
    			}
			else if (strcmp(token, "$$")== 0) {
				token = strtok(NULL, " ");
				printf("%d\n", pid);
			}
    			else {	
    				userInput[i] = strdup(token);
    			}
    			token = strtok(NULL, " ");
    		}


        	if (userInput[0] == "") {
            		continue;
        	}


        	else if (strcmp(userInput[0], "exit") == 0) {
			looping = 0;
		}

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

        	else if (userInput[0][0] == '#') {
            		continue;
        	}

		else if (strcmp(userInput[0], "status") == 0) {
			if (WIFEXITED(exitStatus)) {
				printf("Exit Value %d\n", WEXITSTATUS(exitStatus));
			}
			else {
				printf("Terminated By Signal %d\n", WTERMSIG(exitStatus));
			}
		}
		else {

            		int in, out;
            		pid_t spawnPid = -5;
            		spawnPid = fork();	
            		if (spawnPid == - 1){
                    		perror("Error With Fork\n");
                    		exit(1);
                    		break;
            		}
            		if (spawnPid == 0){	
				ignore.sa_handler = SIG_DFL;
				sigaction(SIGINT, &ignore, NULL);
		                if (strcmp(inputFile, "")) {
                    			in = open(inputFile, O_RDONLY);
                    			if (in == -1) {
                        			perror("Error Opening File\n");
                        			exit(1);
                			}
                    		
					if (dup2(in, 0) == -1) {
                        			perror("Error With DUP2 Function\n");
                        			exit(1);
                    			}
                    			close(in);
            			}
                
				if (strcmp(outputFile, "")) {
                    			out = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    			if (out == -1) {
                        			perror("Error Opening File\n");
                        			exit(1);
                    			}
                    			if (dup2(out, 1) == -1) {
                       	 			perror("Error With DUP2 Function\n");
                        			exit(1);
                    			}
                    			close(out);
                		}

    	        		if (execvp(userInput[0], (char* const*)userInput)) {
                    			printf("Error With %s Command\n", userInput[0]);
					int y;
					for(y = 0; y < 512; y++){
                        			userInput[y] = NULL;
                    			}
                    			exit(1);
                		}
            		}

		    	else{ 
                		if (background == 1 && stopped == 1){	
                    			pid_t actualPid = waitpid(spawnPid, &exitStatus, WNOHANG);	
                    			printf("The Background PID Is %d\n", spawnPid);		
                		}
                		else {
                    			pid_t actualPid = waitpid(spawnPid, &exitStatus, 0);	
                		}
				
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

        inputCommand[0] = '\0';	
        background = 0;		
        inputFile[0] = '\0';	
        outputFile[0] = '\0';	
	int x;
        for(x = 0; x < 512; x++){
            userInput[x] = NULL;
        }
    }
    return 0;
}
