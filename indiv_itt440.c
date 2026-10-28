#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <signal.h>     
#include <sys/types.h>  
#include <sys/wait.h>   

#define NUM_CHILDREN 3    
#define MAX_MSG 256  

static volatile sig_atomic_t got_sigint = 0;
//interrupt_handler()
static void interrupt_handler(int sig)
{
    const char *msg = "\n[INTERRUPT] Ctrl+C (SIGINT) detected - "
                      "program is terminating safely.\n";
    (void)sig;                                  
    write(STDOUT_FILENO, msg, strlen(msg));     
    got_sigint = 1;                             
}

int main(void)
{
    int  pfd[NUM_CHILDREN][2];  
    pid_t pid[NUM_CHILDREN];    
    char buf[MAX_MSG];          
    struct sigaction sa;        
    int i, j;                   
// install the SIGINT handler.
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = interrupt_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    for (i = 0; i < NUM_CHILDREN; i++) {
        if (pipe(pfd[i]) == -1) {               
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }
//fork the children.
    for (i = 0; i < NUM_CHILDREN; i++) {
        pid[i] = fork();
        if (pid[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid[i] == 0) {                       
            ssize_t n;

            signal(SIGINT, SIG_IGN);

            for (j = 0; j < NUM_CHILDREN; j++) {
                close(pfd[j][1]);
                if (j != i) {
                    close(pfd[j][0]);
                }
            }

            printf("Child %d (PID %d) ready.\n", i + 1, getpid());
            fflush(stdout);                      
//read each message from the pipe and print it.
            while ((n = read(pfd[i][0], buf, sizeof(buf) - 1)) > 0) {
                buf[n] = '\0';
                printf("Child %d (PID %d) received: %s", i + 1, getpid(), buf);
                fflush(stdout);
            }

            close(pfd[i][0]);                    
            printf("Child %d (PID %d) exiting.\n", i + 1, getpid());
            exit(EXIT_SUCCESS);                  
        }

        close(pfd[i][0]); 
    }

    printf("Parent (PID %d) started %d children.\n", getpid(), NUM_CHILDREN);

    /* CLEANUP 1: Give children 1 second to print their "ready" messages 
       before the parent prints the first "Message>" prompt. */
    sleep(1);
//read messages from the keyboard in the parent and broadcast each one to every child via its own pipe.
    while (!got_sigint) {
        printf("\nMessage> "); // Added \n to separate inputs visually
        fflush(stdout);                          

        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            break;                               
        }
        if (buf[0] == '\n') {
            continue;                            
        }

        for (i = 0; i < NUM_CHILDREN; i++) {
            if (write(pfd[i][1], buf, strlen(buf)) == -1) {
                perror("write");
            }
        }
        usleep(500000); 
    }
    //graceful shutdown
    for (i = 0; i < NUM_CHILDREN; i++) {
        close(pfd[i][1]);
    }
    for (i = 0; i < NUM_CHILDREN; i++) {
        waitpid(pid[i], NULL, 0);
    }
    printf("\nParent: all children have terminated. Goodbye.\n");
    return 0;
}