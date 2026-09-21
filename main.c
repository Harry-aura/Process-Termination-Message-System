#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define NUM_CHILDREN 3

void print_separator() {
    printf("------------------------------------------------------------\n");
}

int main() {
    pid_t pids[NUM_CHILDREN];
    printf("\n=== Process Termination Message System (PTMS) ===\n");
    printf("[Parent] PID: %d is setting up child tasks...\n", getpid());
    print_separator();

    // Child 1: Normal clean termination
    if ((pids[0] = fork()) == 0) {
        printf("  [Child 1] PID: %d started -> Task: Quick calculation\n", getpid());
        sleep(1);
        printf("  [Child 1] PID: %d finished successfully. Exiting with code 0.\n", getpid());
        exit(0);
    }

    // Child 2: Termination with an explicit error code
    if ((pids[1] = fork()) == 0) {
        printf("  [Child 2] PID: %d started -> Task: Simulated disk I/O failure\n", getpid());
        sleep(2);
        printf("  [Child 2] PID: %d encountered an error! Exiting with code 42.\n", getpid());
        exit(42);
    }

    // Child 3: Abnormal termination via signal (SIGSEGV / Segmentation Fault)
    if ((pids[2] = fork()) == 0) {
        printf("  [Child 3] PID: %d started -> Task: Memory operation (triggering SIGSEGV)\n", getpid());
        sleep(3);
        int *bad_ptr = NULL;
        *bad_ptr = 999; // Causes SIGSEGV
        exit(1);
    }

    printf("[Parent] All %d children spawned. Waiting to reap termination statuses...\n", NUM_CHILDREN);
    print_separator();

    int status;
    pid_t terminated_pid;
    int reaped_count = 0;

    // Reap all children using waitpid to prevent zombies
    while (reaped_count < NUM_CHILDREN) {
        terminated_pid = waitpid(-1, &status, 0);

        if (terminated_pid > 0) {
            reaped_count++;
            printf("[EVENT] Reaped Process PID: %d\n", terminated_pid);

            if (WIFEXITED(status)) {
                printf("  Status: TERMINATED NORMALLY\n");
                printf("  Exit Code: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                printf("  Status: ABNORMALLY TERMINATED (Killed by Signal)\n");
                printf("  Signal Number: %d (%s)\n", sig, strsignal(sig));
#ifdef WCOREDUMP
                if (WCOREDUMP(status)) {
                    printf("  Core Dumped: Yes\n");
                }
#endif
            } else if (WIFSTOPPED(status)) {
                printf("  Status: STOPPED by signal %d\n", WSTOPSIG(status));
            }
            print_separator();
        } else {
            perror("waitpid error");
            break;
        }
    }

    printf("[Parent] All children successfully reaped. No zombies left.\n");
    printf("=== Process Termination Message System Completed ===\n\n");
    return 0;
}
