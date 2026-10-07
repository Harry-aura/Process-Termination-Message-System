# Process Termination Message System (PTMS)

A Linux-based C system programming project that monitors child processes, decodes their termination statuses, and prevents zombie processes.

## Features
- Process creation using `fork()`.
- Process synchronization and status harvesting using `waitpid()`.
- Inspects termination status macros:
  - `WIFEXITED` & `WEXITSTATUS` for clean / exit-code terminations.
  - `WIFSIGNALED` & `WTERMSIG` for abnormal terminations (e.g., `SIGSEGV`).
- Prevents zombie process accumulation.

## Build & Run
```bash
make
./ptms
```

## SafeExec AI Screenshots

SafeExec AI runs untrusted C code in a kernel sandbox with CPU and RAM limits, then reports how the process terminated and confirms no zombie processes remain.

### Clean Exit (Exit 0)
![Clean exit](docs/screenshots/clean-exit.png)

### CPU Time Limit Exceeded (SIGXCPU, Sig 24)
![CPU limit](docs/screenshots/cpu-limit.png)

### Memory Ceiling Enforced (malloc returns NULL, Exit 104)
![Memory limit](docs/screenshots/memory-limit.png)

### Segmentation Fault (SIGSEGV, Sig 11)
![Segfault](docs/screenshots/segfault.png)

### Division by Zero (SIGFPE, Sig 8)
![Division by zero](docs/screenshots/division-by-zero.png)

### Wall-Clock Watchdog Kill (SIGKILL, Sig 9)
![Watchdog kill](docs/screenshots/watchdog-kill.png)
