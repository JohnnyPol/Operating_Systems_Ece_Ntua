#
# Makefile
#

CC = gcc

# CAUTION: Always use '-pthread' when compiling POSIX threads-based
# applications, instead of linking with "-lpthread" directly.
CFLAGS = -Wall -O2 -pthread
LIBS =

all: pthread-test simplesync-mutex simplesync-atomic assembly-atomic.asm assembly-mutex.asm original_simplesync mandel mandel_condition mandel_semaphore

## Pthread test
pthread-test: pthread-test.o
	$(CC) $(CFLAGS) -o pthread-test pthread-test.o $(LIBS)

pthread-test.o: pthread-test.c
	$(CC) $(CFLAGS) -c -o pthread-test.o pthread-test.c


## Original simple sync
original_simplesync: original_simplesync.o
	$(CC) $(CFLAGS) -o original_simplesync original_simplesync.o $(LIBS)

orioriginal_simplesync.o: original_simplesync.c
	$(CC) $(CFLAGS) -c -o original_simplesync.o original_simplesync.c


## Simple sync (two versions)
simplesync-mutex: simplesync-mutex.o
	$(CC) $(CFLAGS) -o simplesync-mutex simplesync-mutex.o $(LIBS)

simplesync-atomic: simplesync-atomic.o
	$(CC) $(CFLAGS) -o simplesync-atomic simplesync-atomic.o $(LIBS)

simplesync-mutex.o: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_MUTEX -c -o simplesync-mutex.o simplesync.c

simplesync-atomic.o: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_ATOMIC -c -o simplesync-atomic.o simplesync.c


## Assembly simplesync
assembly-atomic.asm: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_ATOMIC -S -g -o assembly-atomic.asm simplesync.c
assembly-mutex.asm: simplesync.c
	$(CC) $(CFLAGS) -DSYNC_MUTEX -S -g -o assembly-mutex.asm simplesync.c


## Mandel using semaphores
mandel_semaphore: mandel-lib.o mandel_semaphore.o
	$(CC) $(CFLAGS) -o mandel_semaphore mandel-lib.o mandel_semaphore.o $(LIBS)

mandel_semaphore.o: mandel_semaphore.c
	$(CC) $(CFLAGS) -c -o mandel_semaphore.o mandel_semaphore.c $(LIBS)


## Mandel using conditional operations
mandel_condition: mandel-lib.o mandel_condition.o
	$(CC) $(CFLAGS) -o mandel_condition mandel-lib.o mandel_condition.o $(LIBS)

mandel_condition.o: mandel_condition.c
	$(CC) $(CFLAGS) -c -o mandel_condition.o mandel_condition.c $(LIBS)


## Original Mandel
mandel: mandel-lib.o mandel.o
	$(CC) $(CFLAGS) -o mandel mandel-lib.o mandel.o $(LIBS)

mandel-lib.o: mandel-lib.h mandel-lib.c
	$(CC) $(CFLAGS) -c -o mandel-lib.o mandel-lib.c $(LIBS)

mandel.o: mandel.c
	$(CC) $(CFLAGS) -c -o mandel.o mandel.c $(LIBS)

clean:
	rm -f *.s *.o pthread-test simplesync-atomic simplesync-mutex assembly-atomic.asm assembly-mutex.asm original_simplesync mandel mandel_condition mandel_semaphore 