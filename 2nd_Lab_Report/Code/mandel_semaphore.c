#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h> // Include the header for intptr_t
#include "mandel-lib.h"

/*------------------------------------------------------*/
/*>> Our Code <<*/
// Error handling macro for pthread functions
#define perror_pthread(ret, msg) \
    do                           \
    {                            \
        errno = ret;             \
        perror(msg);             \
    } while (0)
/*------------------------------------------------------*/

#define MANDEL_MAX_ITERATION 100000

/*
 * Output at the terminal is is x_chars wide by y_chars long.
 */
int y_chars = 50;
int x_chars = 140;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin).
 */
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
    /*
     * x and y traverse the complex plane.
     */
    double x, y;

    int n;
    int val;

    /* Find out the y value corresponding to this line */
    y = ymax - ystep * line;

    /* and iterate for all points on this line */
    for (x = xmin, n = 0; n < x_chars; x += xstep, n++)
    {

        /* Compute the point's color value */
        val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
        if (val > 255)
            val = 255;

        /* And store it in the color_val[] array */
        val = xterm_color(val);
        color_val[n] = val;
    }
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
    int i;

    char point = '@';
    char newline = '\n';

    for (i = 0; i < x_chars; i++)
    {
        /* Set the current color, then output the point */
        set_xterm_color(fd, color_val[i]);
        if (write(fd, &point, 1) != 1)
        {
            perror("compute_and_output_mandel_line: write point");
            exit(1);
        }
    }

    /* Now that the line is done, output a newline character */
    if (write(fd, &newline, 1) != 1)
    {
        perror("compute_and_output_mandel_line: write newline");
        exit(1);
    }
}

/*------------------------------------------------------*/
/*>> Our Code <<*/

int safe_atoi(char *s, int *val)
{
    long l;
    char *endp;

    l = strtol(s, &endp, 10);
    if (s != endp && *endp == '\0')
    {
        *val = l;
        return 0;
    }
    else
        return -1;
}

void *safe_malloc(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL)
    {
        fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
                size);
        exit(1);
    }

    return p;
}

// Signal handler for SIGINT
void sigint_handler(int signum)
{
    reset_xterm_color(1);
    exit(1);
}

sem_t *semaphores; // Array of semaphores
int NTHREADS;      // Number of threads (global so functions have access)

// Function to compute Mandelbrot lines in parallel and output in order (synchronization)
void *compute_and_output_mandel_line(void *thread_id)
{
    // Avoiding ... warning
    intptr_t thread_id_ptr = (intptr_t)thread_id;
    int color_val[x_chars];

    for (int i = (int)thread_id_ptr; i < y_chars; i += NTHREADS)
    {
        compute_mandel_line(i, color_val); // Pararell computation
        if (sem_wait(&semaphores[(int)thread_id_ptr]))
        {
            perror("sem_wait");
            exit(1);
        }
        output_mandel_line(1, color_val); // Critical section
        if (sem_post(&semaphores[((int)thread_id_ptr + 1) % NTHREADS]))
        {
            perror("sem_post");
            exit(1);
        }
    }
    return NULL;
}
/*------------------------------------------------------*/

int main(int argc, char **argv)
{
    // Variable used for error handling
    int ret;

    xstep = (xmax - xmin) / x_chars;
    ystep = (ymax - ymin) / y_chars;

    if (argc != 2 || safe_atoi(argv[1], &NTHREADS) < 0 || NTHREADS <= 0)
    {
        fprintf(stderr, "Usage: %s <thread_count>\nthread_count > 0", argv[0]);
        exit(1);
    }

    // Register signal handler
    signal(SIGINT, sigint_handler);

    // Allocate memory for semaphores
    semaphores = safe_malloc(NTHREADS * sizeof(sem_t));

    /*
     * Initialize the first semaphore with an initial value of 1.
     * This semaphore ensures that the first thread (with index 0)
     * can acquire the semaphore immediately without waiting,
     * allowing it to start executing its critical section
     */
    if (sem_init(&semaphores[0], 0, 1))
    {
        perror("sem_init");
        exit(1);
    }

    /*
     * Initialize the remaining semaphores with an initial value of 0.
     * These semaphores will block threads trying to acquire them
     * until they are explicitly released by other threads.
     * This mechanism enables thread synchronization, ensuring that
     * each thread waits for the previous thread to finish its
     * critical section before proceeding.
     */
    for (int i = 1; i < NTHREADS; i++)
    {
        if (sem_init(&semaphores[i], 0, 0) < 0)
        {
            perror("sem_init");
            exit(1);
        }
    }
    // Create threads
    pthread_t thread[NTHREADS];
    for (int i = 0; i < NTHREADS; i++)
    {
        // Avoiding ... warning
        intptr_t thread_id = (intptr_t)i;
        ret = pthread_create(&thread[i], NULL, compute_and_output_mandel_line, (void *)thread_id);
        if (ret)
        {
            perror_pthread(ret, "pthread_create");
            exit(1);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < NTHREADS; i++)
    {
        ret = pthread_join(thread[i], NULL);
        if (ret)
        {
            perror_pthread(ret, "pthread_join");
            exit(1);
        }
    }

    // Destroy the semaphores
    for (int i = 0; i < NTHREADS; i++)
    {
        if (sem_destroy(&semaphores[i]))
        {
            perror("sem_destroy");
            exit(1);
        }
    }

    // Free allocated memory
    free(semaphores);

    // Reset the terminal color
    reset_xterm_color(1);
    return 0;
}