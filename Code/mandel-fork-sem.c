/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <signal.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
sem_t *semaphores;
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
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
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

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
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}
int safe_atoi(char *s,int *val){//turns string to long int (by using strtol)
	long l;
	char *endp;

	l=strtol(s,&endp,10);
	if(s!=endp && *endp=='\0'){
		*val=l;
		return 0;
	}
	else
		return -1;
}
void sigint_handler(int signum)
{
        reset_xterm_color(1);//included in the mandel-lib.c file
        exit(1);
}



void compute_and_output_mandel_line(int line , int NPROCS){
	int color_val[x_chars];
	int i;
	for(i=line; i<y_chars; i+=NPROCS){
		compute_mandel_line(i, color_val);
		if(sem_wait(&semaphores[line])<0){
			perror("sem_wait");
			exit(EXIT_FAILURE);
		}
		output_mandel_line(1, color_val);
		if(sem_post(&semaphores[(i+1)  % NPROCS])<0){
                	perror("sem_post");
                	exit(EXIT_FAILURE);
        	}
	}
}

/*
 * Create a shared memory area, usable by all descendants of the calling
 * process.
 */

void *create_shared_memory_area(unsigned int numbytes)
{
	int pages;
	void *addr;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	/* Create a shared, anonymous mapping for this number of pages */
	addr=mmap(NULL,pages*sysconf(_SC_PAGE_SIZE),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if(addr==MAP_FAILED){
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	return addr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
	int pages;

	if (numbytes == 0) {
		fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
		exit(1);
	}

	/*
	 * Determine the number of pages needed, round up the requested number of
	 * pages
	 */
	
	pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

	if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
		perror("destroy_shared_memory_area: munmap failed");
		exit(1);
	}
}

int main(int argc, char *argv[])
{	
        int i,line, NPROCS, status;

        xstep=(xmax-xmin)/x_chars;
        ystep=(ymax-ymin)/y_chars;

        /*
         * signal handling
	*/

        if(argc!=2)
                printf("Number of arguments provided false");
        if (safe_atoi(argv[1], &NPROCS)<0 || NPROCS<=0) {
                fprintf(stderr, "`%s' is not valid for `processes_count'\n", argv[1]);//stderr is specified so the 
                                                                                     //output of fprint is directed to                                                                                     // std error stream
                exit(1);
        }

        semaphores=create_shared_memory_area(NPROCS * sizeof(sem_t)); 
//initialize the first semaphore with starting value 1(3rd_arg) and specify that 
// the semaphore will be shared between processes(2nd arg is 1)
	if(sem_init(&semaphores[0],1,1)<0){
		perror("sem_init");
		exit(EXIT_FAILURE);
	}
	for(i=1; i<NPROCS; i++){//initialized the rest of the semaphores with value=0
		if(sem_init(&semaphores[i],1,0)<0){
			perror("sem_init");
			exit(EXIT_FAILURE);
		}
	}
	pid_t child_pid[NPROCS];
	for(line=0; line<NPROCS; line++){//create NPROCS processes
		child_pid[line]=fork();
		if(child_pid[line]<0){
			perror("fork failed");
			exit(EXIT_FAILURE);
		}
		if(child_pid[line]==0){
			compute_and_output_mandel_line(line,NPROCS);
			exit(EXIT_SUCCESS);
		}
	}

	for(i=0; i<NPROCS; i++){//wait for all children to terminate
		pid_t terminated_child_pid= wait(&status);
		if(terminated_child_pid<0){
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
	
	for(i=0; i<NPROCS; i++){//destroys the semaphore at the address &semaphores[i]
		if(sem_destroy(&semaphores[i])<0){
		perror("sem_destroy");
		exit(EXIT_FAILURE);
		}
	}

	destroy_shared_memory_area(semaphores, NPROCS*sizeof(sem_t));

	reset_xterm_color(1);
	return 0;
}
