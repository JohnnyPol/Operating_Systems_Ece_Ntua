#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
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
    int fpr, fpw;
    char cc, c2c = 'a';
    int count = 0;

    // Check if the correct number of command-line arguments is provided
    if (argc != 4)
    {
        fprintf(stderr, "Incorrect Syntax\n");
        fprintf(stderr, "Usage: %s <input_file> <output_file> <character_to_search>\n", argv[0]);
        fprintf(stderr, "Example: %s input.txt output.txt a\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Define and set value for variable x in the parent process
    int x = 10;
    printf("Parent process before creating a child process: x = %d\n", x);

    // Fork a child process
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    { // Child process
        // Change value for variable x in the child process
        x = 20;
        printf("Child process: x = %d\n", x);
        printf("Child process: ID = %d, Parent ID = %d\n", getpid(), getppid());
        exit(EXIT_SUCCESS);
    }
    else
    { // Parent process
        x = 30;
        printf("Parent process: x = %d\n", x);
        printf("Parent process: ID = %d, Child ID = %d\n", getpid, pid);

        // Wait for the child process to terminate
        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        explain_wait_status(wpid, status);

        // Proceed with the rest of the code
        int fpr, fpw;
        char cc, c2c = 'a';
        int count = 0;

        // open file for reading only
        if ((fpr = open(argv[1], O_RDONLY)) == -1)
        {
            perror("Error opening input file");
            exit(EXIT_FAILURE); // Terminate the program with a failure status
        }

        // open file for writing the result. If the file doesn't exist create it
        if ((fpw = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
        {
            perror("Error opening output file");
            close(fpr); // Close the input file
            exit(EXIT_FAILURE);
        }

        // character to search for (third parameter in command line)
        c2c = argv[3][0];

        // count the occurrences of the given character
        char buffer[4096];
        while (1)
        {
            ssize_t n = read(fpr, buffer, sizeof(buffer));
            if (n == -1)
            {
                perror("Error reading file");
                exit(EXIT_FAILURE);
            }
            else if (n == 0)
                break;
            else
                for (int i = 0; i < n; i++)
                    if (buffer[i] == c2c)
                        count++;
        }

        // close the file for reading
        if (close(fpr) == -1)
        {
            perror("Problem closing file");
            exit(EXIT_FAILURE);
        }

        // write the result in the output file
        char result[150];
        snprintf(result, sizeof(result), "The character '%c' appears %d times in file %s.\n", c2c, count, argv[1]);
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

        exit(EXIT_SUCCESS); // Terminate the program with a success status
    }

    return 0;
}
