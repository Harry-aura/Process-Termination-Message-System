#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>

typedef struct {
    const char *binary_path;
    int cpu_limit_sec;
    int memory_limit_mb;
} EngineConfig;

static pid_t g_child_pid = 0;

void handle_wallclock_timeout(int sig) {
    (void)sig;
    if (g_child_pid > 0) {
        // Kill the child immediately if the wall-clock deadline is breached
        kill(g_child_pid, SIGKILL);
    }
}

void apply_limits_and_exec(const EngineConfig *cfg) {
    // 1. Redirect child output to /dev/null to keep JSON telemetry stream clean
    int dev_null = open("/dev/null", O_WRONLY);
    if (dev_null >= 0) {
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        close(dev_null);
    }

    // 2. Hardware CPU Time Ceiling (RLIMIT_CPU)
    struct rlimit cpu_lim;
    cpu_lim.rlim_cur = cfg->cpu_limit_sec;
    cpu_lim.rlim_max = cfg->cpu_limit_sec + 1;
    setrlimit(RLIMIT_CPU, &cpu_lim);

    // 3. Virtual Address Space Ceiling (RLIMIT_AS)
    struct rlimit mem_lim;
    rlim_t mem_bytes = (rlim_t)cfg->memory_limit_mb * 1024 * 1024;
    mem_lim.rlim_cur = mem_bytes;
    mem_lim.rlim_max = mem_bytes;
    setrlimit(RLIMIT_AS, &mem_lim);

    // 4. Output Quota (RLIMIT_FSIZE) - Cap max output/file generation to 4 MB
    struct rlimit fsize_lim;
    fsize_lim.rlim_cur = 4 * 1024 * 1024;
    fsize_lim.rlim_max = 4 * 1024 * 1024;
    setrlimit(RLIMIT_FSIZE, &fsize_lim);

    // 5. Fork-Bomb Isolation (RLIMIT_NPROC)
    struct rlimit proc_lim;
    proc_lim.rlim_cur = 0;
    proc_lim.rlim_max = 0;
    setrlimit(RLIMIT_NPROC, &proc_lim);

    execl(cfg->binary_path, cfg->binary_path, (char *)NULL);
    exit(127);
}

int main(int argc, char *argv[]) {
    if (argc < 4) return 1;

    EngineConfig cfg = { argv[1], atoi(argv[2]), atoi(argv[3]) };
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);

    // Set wall-clock watchdog: CPU limit + 1 second buffer for sleeps/blocking I/O
    signal(SIGALRM, handle_wallclock_timeout);
    alarm(cfg.cpu_limit_sec + 2);

    pid_t child_pid = fork();
    if (child_pid < 0) return 1;

    if (child_pid == 0) {
        apply_limits_and_exec(&cfg);
    } else {
        g_child_pid = child_pid;
        int status = 0;
        pid_t harvested = waitpid(child_pid, &status, 0);
        alarm(0); // Cancel watchdog timer once child is reaped
        gettimeofday(&t_end, NULL);

        double elapsed_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                            (t_end.tv_usec - t_start.tv_usec) / 1000.0;

        printf("{\n");
        printf("  \"supervisor_pid\": %d,\n", getpid());
        printf("  \"child_pid\": %d,\n", harvested);
        printf("  \"elapsed_ms\": %.2f,\n", elapsed_ms);

        if (WIFEXITED(status)) {
            int code = WEXITSTATUS(status);
            printf("  \"mode\": \"NORMAL_EXIT\",\n");
            printf("  \"code\": %d,\n", code);
            printf("  \"signal\": 0,\n");
            printf("  \"signal_name\": \"NONE\",\n");
            printf("  \"verdict\": \"%s\"\n", (code == 0) ? "SAFE_EXECUTION_COMPLETED" : "APPLICATION_ERROR");
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            printf("  \"mode\": \"SIGNAL_TERMINATION\",\n");
            printf("  \"code\": %d,\n", 128 + sig);
            printf("  \"signal\": %d,\n", sig);
            printf("  \"signal_name\": \"%s\",\n", strsignal(sig));
            if (sig == SIGXCPU) {
                printf("  \"verdict\": \"HARDWARE_KILL: CPU Quota Exceeded (Infinite Loop Defeated)\"\n");
            } else if (sig == SIGSEGV) {
                printf("  \"verdict\": \"HARDWARE_KILL: Memory Page Violation / RAM Ceiling Breached\"\n");
            } else if (sig == SIGFPE) {
                printf("  \"verdict\": \"HARDWARE_KILL: Arithmetic Division-by-Zero Exception\"\n");
            } else if (sig == SIGKILL) {
                printf("  \"verdict\": \"WATCHDOG_KILL: Wall-Clock Deadline Breached (Process Stalled/Slept)\"\n");
            } else if (sig == SIGXFSZ) {
                printf("  \"verdict\": \"SECURITY_KILL: Output Quota Exceeded (Output Flood Defeated)\"\n");
            } else {
                printf("  \"verdict\": \"HARDWARE_KILL: Intercepted Signal %d\"\n", sig);
            }
        }
        printf("}\n");
    }
    return 0;
}
