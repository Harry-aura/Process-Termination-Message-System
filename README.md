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
