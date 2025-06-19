#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    // TODO Task 0: Tokenize string s
    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error
    if (s == NULL) {
        fprintf(stderr, "Input string is null\n");
        return -1;
    }

    if (tokens == NULL) {
        fprintf(stderr, "Tokens vector pointer is null\n");
        return -1;
    }

    char *token = strtok(s, " ");
    while (token != NULL) {
        if (strvec_add(tokens, token) != 0) {
            fprintf(stderr, "Failed to add token \"%s\"\n", token);
            return -1;
        }
        token = strtok(NULL, " ");
    }
    return 0;
}

int run_command(strvec_t *tokens) {
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL
    if (tokens == NULL || tokens->length == 0) {
        fprintf(stderr, "Error tokens vector is empty\n");
        return -1;
    }

    int index1 = strvec_find(tokens, "<");

    if (index1 != -1) {
        if (index1 + 1 >= tokens->length) {
            fprintf(stderr, "Error no input file given\n");
            return -1;
        }
        char *file1 = strvec_get(tokens, index1 + 1);
        if (file1 == NULL) {
            fprintf(stderr, "Error failed to get input file token\n");
            return -1;
        }
        int fd = open(file1, O_RDONLY);
        if (fd < 0) {
            perror("Failed to open input file");
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 input");
            if (close(fd) < 0) {
                perror("close");
            }
            return -1;
        }
        close(fd);
    }

    int index2 = strvec_find(tokens, ">");
    if (index2 != -1) {
        if (index2 + 1 >= tokens->length) {
            fprintf(stderr, "Error no output file given\n");
            return -1;
        }
        char *file2 = strvec_get(tokens, index2 + 1);
        if (file2 == NULL) {
            fprintf(stderr, "Error failed to get output file\n");
            return -1;
        }
        int fd = open(file2, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            perror("Failed to open output file");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            if (close(fd) < 0) {
                perror("close");
            }

            return -1;
        }
        close(fd);
    }

    int index3 = strvec_find(tokens, ">>");
    if (index3 != -1) {
        if (index3 + 1 >= tokens->length) {
            fprintf(stderr, "Error no output file given\n");
            return -1;
        }
        char *outfile = strvec_get(tokens, index3 + 1);
        if (outfile == NULL) {
            fprintf(stderr, "error couldn't output in append\n");
            return -1;
        }
        int fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            perror("Failed to open output file");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 append");
            close(fd);
            return -1;
        }
        if (close(fd) < 0) {
            perror("close");
        }
    }

    char *args[MAX_ARGS + 1];
    int i = 0, j = 0;
    for (i = 0; i < tokens->length; i++) {
        if (j >= MAX_ARGS) {
            break;
        }
        char *token = strvec_get(tokens, i);
        if (token == NULL) {
            fprintf(stderr, "Error token at index %d is NULL\n", i);
            return -1;
        }
        if (strcmp(token, "<") == 0 || strcmp(token, ">") == 0 || strcmp(token, ">>") == 0) {
            break;
        }
        args[j++] = token;
    }

    args[j] = NULL;
    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    if (sigfillset(&sa.sa_mask) == -1) {
        perror("sigfillset");
        exit(1);
    }
    sa.sa_flags = 0;
    if (sigaction(SIGTTIN, &sa, NULL) == -1 || sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    pid_t child_pid = getpid();

    if (setpgid(child_pid, child_pid) == -1) {
        perror("setpgid");
        exit(1);
    }
    execvp(args[0], args);
    perror("exec");
    return -1;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    // TODO Task 5: Implement the ability to resume stopped jobs in the foreground
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    //    Feel free to use sscanf() or atoi() to convert this string to an int
    // 2. Call tcsetpgrp(STDIN_FILENO, <job_pid>) where job_pid is the job's process ID
    // 3. Send the process the SIGCONT signal with the kill() system call
    // 4. Use the same waitpid() logic as in main -- don't forget WUNTRACED
    // 5. If the job has terminated (not stopped), remove it from the 'jobs' list
    // 6. Call tcsetpgrp(STDIN_FILENO, <shell_pid>). shell_pid is the *current*
    //    process's pid, since we call this function from the main shell process

    if (tokens->length < 2) {
        fprintf(stderr, "no job index\n");
        return -1;
    }
    const char *job_index_str = strvec_get(tokens, 1);
    if (job_index_str == NULL) {
        fprintf(stderr, "Job index token is null\n");
        return -1;
    }

    int job_index = atoi(job_index_str);
    if (job_index == 0 && strcmp(job_index_str, "0") != 0) {
        fprintf(stderr, "Invalid job index: %s\n", job_index_str);
        return -1;
    }

    job_t *job = job_list_get(jobs, job_index);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    if (is_foreground) {
        if (tcsetpgrp(STDIN_FILENO, job->pid) == -1) {
            perror("tcsetpgrp");
            return -1;
        }
        if (kill(job->pid, SIGCONT) == -1) {
            perror("kill");
            return -1;
        }
        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1) {
            perror("waitpid");
            return -1;
        }
        if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) {
            perror("tcsetpgrp restore shell");
            return -1;
        }
        if (!WIFSTOPPED(status)) {
            if (job_list_remove(jobs, job_index) != 0) {
                fprintf(stderr, "Failed to remove job\n");
            }
        }
    }
    // TODO Task 6: Implement the ability to resume stopped jobs in the background.
    // This really just means omitting some of the steps used to resume a job in the foreground:
    // 1. DO NOT call tcsetpgrp() to manipulate foreground/background terminal process group
    // 2. DO NOT call waitpid() to wait on the job
    // 3. Make sure to modify the 'status' field of the relevant job list entry to BACKGROUND
    //    (as it was STOPPED before this)
    else {
        if (kill(job->pid, SIGCONT) == -1) {
            perror("kill SIGCONT");
            return -1;
        }
        job->status = BACKGROUND;
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    // TODO Task 6: Wait for a specific job to stop or terminate
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    // 2. Make sure the job's status is BACKGROUND (no sense waiting for a stopped job)
    // 3. Use waitpid() to wait for the job to terminate, as you have in resume_job() and main().
    // 4. If the process terminates (is not stopped by a signal) remove it from the jobs list
    if (tokens->length < 2) {
        fprintf(stderr, "No job index\n");
        return -1;
    }
    const char *job_index_str = strvec_get(tokens, 1);
    if (job_index_str == NULL) {
        fprintf(stderr, "Job index token is NULL\n");
        return -1;
    }

    int job_index = atoi(job_index_str);
    if (job_index == 0 && strcmp(job_index_str, "0") != 0) {
        fprintf(stderr, "Invalid job index: %s\n", job_index_str);
        return -1;
    }

    job_t *job = job_list_get(jobs, job_index);
    if (job == NULL) {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    if (job->status != BACKGROUND) {
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }
    int status;
    if (waitpid(job->pid, &status, WUNTRACED) == -1) {
        perror("waitpid");
        return -1;
    }

    if (!WIFSTOPPED(status)) {
        if (job_list_remove(jobs, job_index) != 0) {
            fprintf(stderr, "Failed to remove job\n");
            return -1;
        }
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    // TODO Task 6: Wait for all background jobs to stop or terminate
    // 1. Iterate through the jobs list, ignoring any stopped jobs
    // 2. For a background job, call waitpid() with WUNTRACED.
    // 3. If the job has stopped (check with WIFSTOPPED), change its
    //    status to STOPPED. If the job has terminated, do nothing until the
    //    next step (don't attempt to remove it while iterating through the list).
    // 4. Remove all background jobs (which have all just terminated) from jobs list.
    //    Use the job_list_remove_by_status() function.

    job_t *cur = jobs->head;
    int status;

    while (cur != NULL) {
        if (cur->status == BACKGROUND) {
            if (waitpid(cur->pid, &status, WUNTRACED) == -1) {
                perror("waitpid");
                return -1;
            }

            if (WIFSTOPPED(status)) {
                cur->status = STOPPED;
            }
        }
        cur = cur->next;
    }
    job_list_remove_by_status(jobs, BACKGROUND);

    return 0;
}
