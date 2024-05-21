#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
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

int count_char(char *filename, char target_char, off_t start, off_t end, int worker_id)
{

    int fpr, count = 0;
    char cc;
    //  open file for reading only
    if ((fpr = open(filename, O_RDONLY)) == -1)
    {
        fprintf(stderr, "Problem opening file to read\n");
        exit(EXIT_FAILURE);
    }

    // Move to the starting position
    lseek(fpr, start, SEEK_SET);

    // count the occurrences of the given character in the assigned portion of the file
    while (read(fpr, &cc, 1) > 0 && lseek(fpr, 0, SEEK_CUR) <= end)
    {

        if (cc == target_char)
            count++;
    }

    close(fpr);
    return count;
}

int main(int argc, char *argv[])
{
    char *filename = argv[1];
    char target_char = argv[3][0];
    off_t start = atoi(argv[4]);
    off_t end = atoi(argv[5]);
    int worker_id = atoi(argv[6]);
    int fpr;
    char cc;
    //  open file for reading only
    if ((fpr = open(filename, O_RDONLY)) == -1)
    {
        fprintf(stderr, "Problem opening file to read\n");
        exit(EXIT_FAILURE);
    }
    //  Count the occurrences of the given character in the assigned portion of the file
    int count = count_char(filename, target_char, start, end, worker_id);

    write(1, &count, sizeof(int));
    // close the file for reading
    close(fpr);
    return 0;
}