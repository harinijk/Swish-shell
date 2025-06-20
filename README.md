# swish - A Simple Unix Shell

**swish** is a custom-built Unix shell implemented in C. It supports essential shell features like command parsing, process creation, input/output redirection, job control, and foreground/background execution, providing a hands-on experience with low-level systems programming and POSIX system calls.

---

##  Features

- Command execution with support for arguments (`execvp`)
- Built-in commands: `cd`, `pwd`, `jobs`, `fg`, `bg`, `wait-for`, `wait-all`
- Job control: 
  - Foreground and background job management
  - Signal handling (`SIGINT`, `SIGTSTP`, `SIGCONT`)
  - Job suspension and resumption
- Input/output redirection:
  - Overwrite (`>`)
  - Append (`>>`)
  - Input (`<`)
- Background execution using `&`
- Custom data structures for string vectors and job lists
- Error handling and memory/resource cleanup

---

##  Technologies Used

- **Language:** C (POSIX compliant)
- **Tools:** Docker, GDB (for debugging), Linux terminal
- **System Calls:** `fork`, `execvp`, `waitpid`, `dup2`, `setpgid`, `tcsetpgrp`, `sigaction`, etc.

---

##  Testing

To compile and run all test cases:

```bash
make
make test

```
##  Sample Usage

@> ls -l > output.txt

@> cat < input.txt

@> wc -l < input.txt > linecount.txt

@> ./a.out &

@> jobs

@> fg 0

@> wait-for 0



⚙##  Build & Run

make
./swish

## Learnings
This project involved:

Deep understanding of Unix process lifecycle and job control

Managing signals and foreground/background processes

Implementing core shell logic from scratch using low-level system calls


