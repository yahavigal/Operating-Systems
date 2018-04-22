/*
 * mmap_writer.c
 *
 *  Created on: 6 Dec 2016
 *      Author: yahav
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#define PATH "/tmp/mmapped.bin"
#define DEC 10

int main(int argc, char* argv[]){
	char *arr, *ptr;
	int file, num, pid, i, result;
	struct sigaction ignr_sigterm, old_handler;
	struct timeval t1, t2;
	double elapsed_millisec;

	// Assign pointer to our handler function
	ignr_sigterm.sa_handler = SIG_IGN;

	// Remove any special flag
	ignr_sigterm.sa_flags = 0;

	//ignoring SIGTERM
	if (sigaction(SIGTERM, &ignr_sigterm, &old_handler)!=0){
		printf("Signal handle for ignoring SIGTERM registration failed. %s\n",strerror(errno));
		return errno;
	}

	if((file = open(PATH, O_RDWR | O_CREAT, 0600))<0){
    	printf("Error while opening the file. %s\n",strerror(errno));
    	return errno;
    }

	num = strtol(argv[1], &ptr, DEC);
	pid = strtol(argv[2], &ptr, DEC);

	if(file<0){
		printf("Error creating new file: %s\n", strerror(errno));
		return errno;
	}

	// Force the file to be of the same size as the (mmapped) array
	result = lseek(file, num-1, SEEK_SET);
	if (result == -1){
		printf("Error calling lseek() to 'stretch' the file: %s\n", strerror(errno));
		return errno;
	}

	// Something has to be written at the end of the file,
	// so the file actually has the new size.
	result = write(file, "", 1);
	if (result!=1){
		printf("Error writing last byte of the file: %s\n", strerror(errno));
		return errno;
	}

	arr = (char*) mmap(NULL, num, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0); // create mapping
	if (arr == MAP_FAILED){
	    printf("Error mmapping the file: %s\n", strerror(errno));
	    return errno;
	}
	// Start time measuring
	if(gettimeofday(&t1, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}
	for (i=0; i<num-1; i++){
		arr[i] = 'a';
	}
	arr[num-1] = '\0';

	kill(pid, SIGUSR1);

	// Finish time measuring
	if (gettimeofday(&t2, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}
	// Counting time elapsed
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;
	// Final report
	printf("%d bytes written, %f milliseconds through MMAP\n", num, elapsed_millisec);

	//un-ignoring SIGTERM
	if (sigaction(SIGTERM, &old_handler, NULL)!=0){
		printf("Signal handle for ignoring SIGTERM registration failed. %s\n",strerror(errno));
		return errno;
	}

	if (munmap(arr, num)<0){
	   printf("Error un-mmapping the file: %s\n", strerror(errno));
	   return errno;
	}

	// un-mmaping doesn't close the file, so we still need to do that.
	if(close(file)<0){
	   printf("Error while closing the file: %s\n", strerror(errno));
	   return errno;
	}
	return 0;
}
