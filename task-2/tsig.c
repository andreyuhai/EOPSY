#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int INTERRUPT_FLAG = 0;
int NUM_CHILD = 5;
int WITHOUT_SIGNALS = 1;

void child_do();
void send_signal_to_multiple(pid_t* pids, int pids_count, int signal);
void custom_sigint_handler();

int main() {
    pid_t pids[NUM_CHILD]; // Array to keep children PIDs
    pid_t pid;
    pid_t parent_pid = getpid();
    pid_t cpi; // Child process id
    int status = 0; // To keep child process exit statuses

    // Set custom signal handling if WITHOUT_SIGNALS flag is set
    if(WITHOUT_SIGNALS) {
        for(int i = 1; i < SIGRTMAX; i++) {
            signal(i, SIG_IGN);
        }

        // Set SIGCHLD back to DEFAULT
        signal(SIGCHLD, SIG_DFL);

        // Set custom SIGINT handler
        signal(SIGINT, custom_sigint_handler);
    }

    for(int i = 0; i < NUM_CHILD; i++) {
        pid = fork();

        if(pid == 0) {
            child_do();
        } else if(pid == -1) {
            printf("parent[%d]: There was something wrong while forking.\n", parent_pid);
            printf("parent[%d]: Sending SIGTERM to all the created children.\n", parent_pid);
            send_signal_to_multiple(pids, i + 1, SIGTERM);
            exit(1);
        }

        pids[i] = pid;

        if(WITHOUT_SIGNALS) {
            // Check whether INTERRUPT_FLAG has been set
            if(INTERRUPT_FLAG) {
                printf("parent[%d]: Forking process got interrupted, sending SIGTERM to all the children created so far.\n", parent_pid);
                send_signal_to_multiple(pids, i + 1, SIGTERM);
                break;
            }
        }

        sleep(1);
    }

    int child_process_exit_codes[NUM_CHILD];
    int child_process_ids[NUM_CHILD];
    int child_process_counter = 0;
    // Save exit statuses of child processes
    while((cpi = wait(&status)) > 0) {
        child_process_ids[child_process_counter] = cpi;
        child_process_exit_codes[child_process_counter] = status;
        child_process_counter++;
    }

    // Print exit statuses of child processes
    for(int i = 0; i < child_process_counter; i++) {
        printf("parent[%d]: Child [%d] has exited with code [%d]\n",
                parent_pid,
                child_process_ids[i],
                WEXITSTATUS(child_process_exit_codes[i]));
    }

    // Recover signal handling back to defaults
    if(WITHOUT_SIGNALS) {
        for(int i = 1; i < SIGRTMAX; i++) {
            signal(i, SIG_DFL);
        }
    }

    return 0;
}

// Send signals to multiple processes
void send_signal_to_multiple(pid_t* pids, int pids_count, int signal) {
    // Make sure there are already children forked.
    if(pids_count > 0) {
        // Send the signal to each and every one of them.
        for(int i = 0; i < pids_count; i++) {
            kill(pids[i], signal);
        }
    }
}

// Function for child processes
void child_do() {
    printf("child[%d]: I'm a child process and my parent process id is [%d].\n", getpid(), getppid());
    sleep(10);
    printf("child[%d]: Execution has been completed!\n", getpid());

    exit(0);
}

void custom_sigint_handler() {
    printf("parent[%d]: Got an interrupt!\n", getpid());
    INTERRUPT_FLAG = 1;
}
