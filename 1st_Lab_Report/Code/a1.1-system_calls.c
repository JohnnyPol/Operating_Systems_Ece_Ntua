#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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

    // open file for reading only
    if ((fpr = open(argv[1], O_RDONLY)) == -1)
    {
        perror("Problem opening file to read");
        exit(EXIT_FAILURE);
    }

    // open file for writing the result. If the file doesn't exist create it
    if ((fpw = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1)
    {
        perror("Problem opening file to write");
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

    exit(EXIT_SUCCESS);
}