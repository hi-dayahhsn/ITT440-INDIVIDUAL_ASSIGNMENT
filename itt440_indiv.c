#include <stdio.h>	//STANDARD INPUT/OUTPUT (PRINTF, FGETS)
#include <stdlib.h>	//STANDARD LIBRARY (EXIT, SYSTEM UTILITIES)
#include <sys/wait.h>	//PROCESS TERMINATION WAITING (WAIT)
#include <sys/types.h>	//DATA TYPES FOR PROCESS IDS (PID_T)
#include <unistd.h>	//POSIX OS API (FORK, PIPE, READ, WRITE, CLOSED, GETPID)
#include <string.h>	//STRING MANIPULATION (STRLEN)
#include <signal.h>	//POSIX SIGNAL HANDLING (SIGNAL, SIGINT)
#include <errno.h>	//ERROR REPORTING & TRACKING

#define BUFFER_SIZE 512	//DEFINE FOR PASSING TEXT MESSAGE ACROSS THE PIPE

//CUSTOM SIGNAL HANDLER Function FOR CTRL + C (SIGINT = 2)
void handle_sigint(int sig) {
	printf("\n\n=========================================================================================\n");
	printf("			[STUDENT INTERRUPT DETECTED!!!]\n");
	printf("------------------------------------------------------------------------------------------\n");
	printf("Study session paused! Time to stretch, hydrate, take a deep breath & have a quick break.\n");
	printf("Exiting Student Task Manager gracefully...\n");
	printf("==========================================================================================\n");
	exit(0); //TERMINATE THE PROCESS CLEANLY WITH STATUS (0 = SUCCESS)
}

int main() {

	//ARRAY TO HOLD FILE DESCIPTORS FOR PIPE CREATION
	//PIPE_FD[0] -> READ END OF THE PIPE
	//PIPE_FD[1] -> WRITE END OF THE PIPE
	int pipe_fd[2];
	pid_t pid; //VARIABLE TO STORE THE PROCESS ID RETURNED BY FORK()
	char input_buffer[BUFFER_SIZE]; //CHARACTER BUFFER ARRAY TO STORE USER INPUT

	//--------------------------------------------------
	//STEP 1: CREATE PIPE | PIPE() CALLED BEFORE FORK()
	//--------------------------------------------------

	//PIPE() CALLED BEFORE FORK() SO BOTH PARENT & CHILD INHERIT THE EXACT SAME PIPE FD
	if(pipe(pipe_fd) == -1) {
		perror("[ERROR] Pipe creation failed");
		return 1;
	}

	//----------------------
	//STEP 2: FORK PROCESS 
	//----------------------

	//FORK() DUPLICATE CURRENT PROCESS
	//RETURN 0 TO THE CHILD PROCESS
	//RETURN CHILD PID > 0 TO PARENT PROCESS
	//RETURN -1 IF PROCESS CREATION FAILS 
	pid = fork();

	if(pid < 0) {
		perror("[ERROR] Fork failed to create child process");
		return 1;
	}

	//-----------------------------------------------------
	//STEP 3: CHILD PROCESS: STUDENT NOTIFICATION SYSTEM (PID == 0)
	//-----------------------------------------------------

	if(pid == 0) {

		//SIGNAL HANDLER BUT PARENT PROCESS HANDLE THE INTERRUPT
		signal(SIGINT, SIG_IGN);

		//CHILD READS FROM THE PIPE (CLOSE UNSED WRITE END)
		close(pipe_fd[1]);

		char rcvd_msg[BUFFER_SIZE];
		int bytes_read;

		printf("\n[CHILD PROCESS - PID %d] Task Logger initialized and waiting for study updates...\n", getpid());

		//CONTINUOUSLY READ MESSAGES PASSED FROM THE PARENT PROCESS @ PIPE
		while((bytes_read = read(pipe_fd[0], rcvd_msg, sizeof(rcvd_msg) -1)) > 0) {
			rcvd_msg[bytes_read] = '\0'; //NULL-TERMINATE STRING

			//DISPLAY MESSAGE ON TERMINAL
			printf("[CHILD DISPLAY] New Study Task Recorded: %s", rcvd_msg);
			fflush(stdout); //PROMPT APPEAR IMMEDIATLY 
		}

		//CLOSE READ END BEFORE EXITING CHILD EXECUTION
 		close(pipe_fd[0]);
		exit(0);
	}

	//------------------------------------------------------
	//STEP 4: PARENT PROCESS: STUDENT INTERACTIVE PLANNER (PID > 0)
	//------------------------------------------------------

	else {
		//TO CATCH INTERRUPT  USING CUSTOM HANDLER
		signal(SIGINT, handle_sigint);

		//PARENT ONLY WRITES TO THE PIPE; (CLOSE UNUSED READ END)
		close(pipe_fd[0]);

		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		printf("   		   ITT440 STUDENT STUDY & TASK MANAGER\n");
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		printf("[PARENT PROCESS - PID %d] Ready for input.\n", getpid());
		printf("Enter your study task below || e.g; 'Complete ITT440 Individual Assignment'\n");
		printf("Press Ctrl+C at any time to take a study break.\n");
		printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");

		//CONTINOUS LOOP TO GET USER INPUT 
		while(1) {
			printf("Enter Task > ");
			fflush(stdout); //PROMPT APPEARS IMMEDIATELY

			//GET MESSAGE INPUT FROM USER
			if(fgets(input_buffer, BUFFER_SIZE, stdin) != NULL) {

				//WRITE THE BUFFER INPUT FROM USER TO CHILD VIA PIPE
				write(pipe_fd[1], input_buffer, strlen(input_buffer));
				sleep(1); //DELAY 1 SEC FOR CLEAN CONSOLE PRINTING OUTPUT
			}
		}

		//CLOSE THE WRITE END OF PIPE AND WAIT FOR CHILD PROCESS COMPLETION
		close(pipe_fd[1]);
		wait(NULL);
	}
	return 0;
}
