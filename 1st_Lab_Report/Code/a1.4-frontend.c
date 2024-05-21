#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "config.h"

void show_pstree(pid_t p)
{
    int ret;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "echo; echo; pstree -a -G -c -p %ld; echo; echo", (long)p);
    cmd[sizeof(cmd) - 1] = '\0';
    ret = system(cmd);
    if (ret < 0)
    {
        perror("system");
        exit(104);
    }
}

void explain_wait_status(pid_t wpid, int status)
{
    if (WIFEXITED(status))
        printf("Dispatcher process %ld terminated normally with exit status: %d\n", (long)wpid, WEXITSTATUS(status));

    else if (WIFSIGNALED(status))
        printf("Dispatcher process %ld terminated by signal: %d\n", (long)wpid, WTERMSIG(status));

    else if (WIFSTOPPED(status))
        printf("Dispatcher process %ld stopped by signal: %d\n", (long)wpid, WSTOPSIG(status));

    else if (WIFCONTINUED(status))
        printf("Dispatcher process %d continued\n", wpid);

    else
        printf("Unknown status for Dispatcher process %d\n", wpid);
}

void handler_SIGINT(int signal)
{
    // Code to handle SIGINT signal
}

void handler_SIGKILL(int signal)
{
    // Code to handle SIGTSTP signal
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

    // Declare the starting number of workers
    int num_workers;
    printf("> Enter number of Workers: ");
    scanf("%d", &num_workers);
    fflush(stdin);

    int fd[2];
    char buffer[BUFFER_SIZE];

    // Install signal handler for SIGINT and SIGKILL
    signal(SIGINT, handler_SIGINT);
    signal(SIGKILL, handler_SIGKILL);

    // Create pipe for communication between frontend and dispatcher
    if (pipe(fd) == -1)
    {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Replace standard output with write end of pipe
        dup2(fd[1], 1);

        // Send the number of workers to the dispatcher through an argument
        char workers[5];
        sprintf(workers, "%d", num_workers);

        // Execute dispatcher
        char *args[] = {"a1.4-dispatcher", argv[1], argv[2], argv[3], workers, NULL};
        execv("a1.4-dispatcher", args);

        // If execv fails, print error message
        perror("Error executing dispatcher");
        exit(EXIT_FAILURE);
    }

    int counter;
    read(fd[0], &counter, sizeof(int));

    // Print the number of times the character appears in the file
    printf("The character '%c' appears %d times in file %s.\n", argv[3][0], counter, argv[1]);

    // Wait for the dispatcher process to terminate
    int status;
    pid_t wpid = waitpid(pid, &status, 0);

    printf("\n...Front-End has terminated...\n");
    return 0;
}
/*    while (1)
    {
        printf("> ");
        scanf("%s", buffer);

        // Send to the dispatcher the number of workers to be created
        if (strncmp(buffer, "add", 3) == 0)
        {
            // Send command to add worker to dispatcher
            write(fd[0], buffer, strlen(buffer));
        }
        else if (strncmp(buffer, "remove", 6) == 0)
        {
            // Send command to remove worker to dispatcher
            write(fd[0], buffer, strlen(buffer));
        }
        else if (strncmp(buffer, "info", 4) == 0)
        {
            // Send command to get information about workers to dispatcher
            write(fd[0], buffer, strlen(buffer));

            // Read information from dispatcher
            read(fd[0], buffer, BUFFER_SIZE);
            printf("%s", buffer);
        }
        else if (strncmp(buffer, "progress", 8) == 0)
        {
            // Send command to get progress information from dispatcher
            write(fd[0], buffer, strlen(buffer));

            // Read progress information from dispatcher
            read(fd[0], buffer, BUFFER_SIZE);
            printf("%s", buffer);
        }
        else if (strncmp(buffer, "quit", 4) == 0)
        {
            // Send command to quit to dispatcher
            write(fd[0], buffer, strlen(buffer));

            // Wait for dispatcher to finish
            int status;
            pid_t wpid = waitpid(pid, &status, 0);
            explain_wait_status(wpid, status);

            // Close read end of pipe
            close(fd[0]);

            break;
        }
    }
 */
