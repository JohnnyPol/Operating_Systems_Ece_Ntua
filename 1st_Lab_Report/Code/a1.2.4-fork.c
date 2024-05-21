#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void explain_wait_status(pid_t wpid, int status)
{
    if (WIFEXITED(status))
        printf("Child process %ld terminated normally with exit status: %d\n", (long)wpid, WEXITSTATUS(status));

    else if (WIFSIGNALED(status))
        printf("Child process %ld terminated by signal: %d\n", (long)wpid, WTERMSIG(status));

    else if (WIFSTOPPED(status))
        printf("Child process %ld stopped by signal: %d\n", (long)wpid, WSTOPSIG(status));

    else if (WIFCONTINUED(status))
        printf("Child process %d continued\n", wpid);

    else
        printf("Unknown status for child process %d\n", wpid);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Incorrect Syntax\n");
        fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_search>\n", argv[0]);
        fprintf(stderr, "Example: %s input.txt output.txt a\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // Child process
        // Create argument list for execv
        char *args[] = {"a1.1-C", argv[1], argv[2], argv[3], NULL};

        // Execute char count child program
        execv("a1.1-C", args);

        // execv returns only if an error occurs
        perror("Execv failed");
        exit(EXIT_FAILURE);
    }
    else
    { // Parent process

        // Wait for the child process to terminate
        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        explain_wait_status(wpid, status);

        printf("Parent process: Child process has terminated.\n");
    }

    return 0;
}
