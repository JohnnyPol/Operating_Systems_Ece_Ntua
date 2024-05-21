#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include "config.h"

void handler_SIGINT(int signal)
{
    // Code to handle SIGINT signal
}

void handler_SIGKILL(int signal)
{
    // Code to handle SIGKILL signal
}

void explain_wait_status(pid_t wpid, int status)
{
    if (WIFEXITED(status))
        fprintf(stderr, "Worker process %ld terminated normally with exit status: %d\n", (long)wpid, WEXITSTATUS(status));

    else if (WIFSIGNALED(status))
        fprintf(stderr, "Worker process %ld terminated by signal: %d\n", (long)wpid, WTERMSIG(status));

    else if (WIFSTOPPED(status))
        fprintf(stderr, "Worker process %ld stopped by signal: %d\n", (long)wpid, WSTOPSIG(status));

    else if (WIFCONTINUED(status))
        fprintf(stderr, "Worker process %d continued\n", wpid);

    else
        fprintf(stderr, "Unknown status for Worker process %d\n", wpid);
}

int main(int argc, char *argv[])
{
    // fprintf(stderr, "This is the Dispatcher:... ");
    char buffer[BUFFER_SIZE];

    int num_workers = atoi(argv[4]);
    if (num_workers > MAX_WORKERS)
        num_workers = MAX_WORKERS;

    int *worker_pids = (int *)malloc(num_workers * sizeof(int));

    // Get the size of the file
    struct stat st;
    if (stat(argv[1], &st) == -1)
    {
        perror("Unable to get file size");
        exit(EXIT_FAILURE);
    }
    off_t file_size = st.st_size;
    if (num_workers > file_size)
        num_workers = file_size;
    // Calculate the portion of the file that each child process will handle
    off_t portion_size = file_size / num_workers;
    off_t start = 0, end = portion_size;

    // Create array for the pipes connecting to the workers
    int **fd_array = (int **)malloc(num_workers * sizeof(int *));
    for (int i = 0; i < num_workers; i++)
        fd_array[i] = (int *)malloc(2 * sizeof(int));

    // Fork worker processes
    for (int i = 0; i < num_workers; i++)
    {
        // Create the pipe connection
        if (pipe(fd_array[i]) == -1)
        {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        // Create the child worker
        worker_pids[i] = fork();

        if (worker_pids[i] < 0)
        {
            perror("Error forking worker process");
            exit(EXIT_FAILURE);
        }
        else if (worker_pids[i] == 0)
        {
            // Execute worker
            // fprintf(stderr, "Dispatcher: Child %d with start-> %d and end-> %d\n", i, start, end);
            char start_byte[10], end_byte[10], worker_id[5];
            sprintf(start_byte, "%d", start);
            sprintf(end_byte, "%d", end);
            sprintf(worker_id, "%d", i);
            char *args[] = {"./a1.4-worker", argv[1], argv[2], argv[3], start_byte, end_byte, worker_id, NULL};
            dup2(fd_array[i][1], 1);
            execvp("./a1.4-worker", args);
            perror("Error executing Worker");
            exit(EXIT_FAILURE);
        }
        close(fd_array[i][1]); // Close write for dispatcher
        // Calculate the portion of the file for the next worker
        start = end;
        end = (i == num_workers - 2) ? file_size - 1 : start + portion_size;
    }
    pid_t pid;

    // Wait for all the workers to finish
    for (int i = 0; i < num_workers; i++)
    {
        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        // Uncomment to see the status of the finished workers
        // explain_wait_status(wpid, status);
    }
    int total = 0;
    for (int i = 0; i < num_workers; i++)
    {
        int local;
        read(fd_array[i][0], &local, sizeof(int));
        total += local;
    }

    write(1, &total, sizeof(int));

    for (int i = 0; i < num_workers; i++)
        free(fd_array[i]);
    free(fd_array);
    free(worker_pids);
}
/*
    while (1)
    {
        // Read command from frontend
        read(fd[0], buffer, BUFFER_SIZE);

        if (strncmp(buffer, "add", 3) == 0)
        {
            if (num_workers == MAX_WORKERS)
            {
                printf("Max number of workers reached\n");
                continue;
            }
            // Fork new worker process
            pid = fork();

            if (pid < 0)
            {
                perror("Error forking worker process");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Execute worker
                char *args[] = {"a1.4-worker", argv[1], argv[2], NULL};
                execv("a1.4-worker", args);
            }
            else
            {
                worker_pids[num_workers] = pid;
                num_workers++;
            }
        }
        else if (strncmp(buffer, "remove", 6) == 0)
        {
            int worker_id = atoi(buffer + 7);

            // Send SIGTERM signal to worker process
            kill(worker_pids[worker_id], SIGTERM);

            // Remove worker process from list
            for (int i = worker_id; i < num_workers - 1; i++)
            {
                worker_pids[i] = worker_pids[i + 1];
            }
            worker_pids[num_workers--] = 0;
        }
        else if (strncmp(buffer, "info", 4) == 0)
        {
            printf("Number of workers: %d\n", num_workers);

            for (int i = 0; i < num_workers; i++)
            {
                printf("Worker %d: PID %d\n", i, worker_pids[i]);
            }
        }
        else if (strncmp(buffer, "progress", 8) == 0)
        {
            int total_chars = 0;
            int chars_found = 0;

            for (int i = 0; i < num_workers; i++)
            {
                // Send SIGKILL signal to worker process to get progress information
                kill(worker_pids[i], SIGKILL);

                // Read progress information from worker process
                read(fd[0], buffer, BUFFER_SIZE);

                sscanf(buffer, "%d %d", &total_chars, &chars_found);
            }

            int progress = (int)((double)chars_found / (double)total_chars * 100.0);

            printf("Progress: %d%%, Characters found: %d\n", progress, chars_found);
        }
        else if (strncmp(buffer, "quit", 4) == 0)
        {
            // Send SIGTERM signal to worker processes
            for (int i = 0; i < num_workers; i++)
            {
                kill(worker_pids[i], SIGTERM);
            }

            // Wait for worker processes to finish
            for (int i = 0; i < num_workers; i++)
            {
                wait(NULL);
            }

            break;
        }
    }
*/
