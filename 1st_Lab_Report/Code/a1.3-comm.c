#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define P 10 // Number of child processes

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

void sigint_handler(int signum)
{
    printf("\nNumber of child processes searching and counting: %d\n", P);
}

int count_char(char *filename, char target_char, off_t start, off_t end)
{
    int fpr, count = 0;
    char cc;
    //  open file for reading only
    if ((fpr = open(filename, O_RDONLY)) == -1)
    {
        printf("Problem opening file to read\n");
        exit(EXIT_FAILURE);
    }

    // Move to the starting position
    lseek(fpr, start, SEEK_SET);

    // count the occurrences of the given character in the assigned portion of the file
    while (read(fpr, &cc, 1) > 0 && lseek(fpr, 0, SEEK_CUR) <= end)
        if (cc == target_char)
            count++;

    // close the file for reading
    if (close(fpr) == -1)
    {
        perror("Problem closing file");
        exit(EXIT_FAILURE);
    }
    return count;
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

    // Install signal handler for SIGINT
    signal(SIGINT, sigint_handler);

    pid_t pid;

    int pipefd[P][2];
    // Create P pipes
    for (int i = 0; i < P; i++)
        if (pipe(pipefd[i]) == -1)
        {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

    // Get the size of the file
    struct stat st;
    if (stat(argv[1], &st) == -1)
    {
        perror("Unable to get file size");
        exit(EXIT_FAILURE);
    }
    off_t file_size = st.st_size;
    // Uncomment to print the file size
    // printf("File size is: %d\n", file_size);
    // Calculate the portion of the file that each child process will handle
    off_t portion_size = file_size / P;
    off_t start, end;

    //  Create P child processes
    for (int i = 0; i < P; i++)
    {
        pid = fork();

        if (pid == -1)
        {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            // Close read end of the pipe
            if (close(pipefd[i][0]) == -1)
            {
                perror("Close read end of pipe failed");
                exit(EXIT_FAILURE);
            }
            // Calculate the portion of the file for this child process
            start = i * portion_size;
            end = (i == P - 1) ? file_size - 1 : start + portion_size;
            int char_count = count_char(argv[1], argv[3][0], start, end);

            // Write char_count to the corresponding pipe
            if (write(pipefd[i][1], &char_count, sizeof(char_count)) == -1)
            {
                perror("Write to pipe failed");
                exit(EXIT_FAILURE);
            }

            // Close write end of the pipe
            if (close(pipefd[i][1]) == -1)
            {
                perror("Close write end of pipe failed");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        }
    }

    // Parent process
    // Wait for all the workers to exit
    for (int i = 0; i < P; i++)
    {
        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        // Uncomment to see the status
        // explain_wait_status(wpid, status);
    }
    // Close write end of the first pipe
    if (close(pipefd[0][1]) == -1)
    {
        perror("Close write end of pipe failed");
        exit(EXIT_FAILURE);
    }
    // Read char_count from each pipe
    int total_count = 0;
    for (int i = 0; i < P; i++)
    {
        int count;
        if (read(pipefd[i][0], &count, sizeof(count)) == -1)
        {
            perror("Read from pipe failed");
            exit(EXIT_FAILURE);
        }
        total_count += count;
        // Uncomment to see how many characters each child counted
        // printf("Parent Process: Counted characters from pipe %d is %d\n", i, count);
        close(pipefd[i][0]); // Close read end of the pipe
    }

    // Write total_count to output file
    int fpw;
    if ((fpw = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
    {
        printf("Problem opening file to write\n");
        exit(EXIT_FAILURE);
    }

    // write the result in the output file
    char result[150];
    snprintf(result, sizeof(result), "The character '%c' appears %d times in file %s.\n", argv[3][0], total_count, argv[1]);
    if (write(fpw, result, strlen(result)) != strlen(result))
    {
        perror("Problem writing to file");
        exit(EXIT_FAILURE);
    }
    // close the output file
    if (close(fpw) == -1)
    {
        perror("Problem closing file");
        exit(EXIT_FAILURE);
    }
    return 0;
}
