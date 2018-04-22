/*
 * mmap_reader.c
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

int usr1_flag = 0;

// Signal handler.
// Turn on the flag - the signal was received
void my_sweet_sweet_handler (int signum){
	usr1_flag = 1;
}

int main(){

	int file, size, cnt=0, i=0;
	char *arr, tmp;
	struct stat buf;
	struct timeval t1, t2;
	double elapsed_millisec;
	struct sigaction sig_handler, ignr_sigterm, old_handler;

	// Assign pointer to our handler function
	sig_handler.sa_handler = my_sweet_sweet_handler;
	ignr_sigterm.sa_handler = SIG_IGN;

	// Remove any special flag
	sig_handler.sa_flags = 0;
	ignr_sigterm.sa_flags = 0;

	//ignoring SIGTERM
	if (sigaction(SIGTERM, &ignr_sigterm, &old_handler)!=0){
		printf("Signal handle for ignoring SIGTERM registration failed. %s\n",strerror(errno));
		return errno;
	}

	// Register the handler
	if (sigaction (SIGUSR1, &sig_handler, NULL)!=0){
	printf("Signal handle registration failed. %s\n",strerror(errno));
	return errno;
	}
	// Meditate until received signal
	while(usr1_flag!=1){
		sleep(2);
	}

	if((file = open(PATH,O_RDWR))<0){
    	printf("Error while opening the file. %s\n",strerror(errno));
    	return errno;
    }

	if (file<0){
	    printf("failed opening from path. %s\n",strerror(errno));
	    return errno;
	}

	//check the size of the file
	if(fstat(file, &buf)<0){
		printf("Error in stat function: %s\n", strerror(errno));
		return errno;
	}
	size = buf.st_size;

	// Start time measuring
	if(gettimeofday(&t1, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}

	// create mapping
	arr = (char*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0); // create mapping
	if (arr == MAP_FAILED){
	    printf("Error mmapping the file: %s\n", strerror(errno));
	    return errno;
	}

	// Init tmp var
	tmp = arr[0];

	//count number of 'a' chars
	while (tmp!='\0'){
		tmp = arr[i];
		if (tmp == 'a')
			cnt++;
		i++;
	}

	if (tmp == '\0')
		cnt++;
	else {
		cnt++;
		printf("%d bytes read through MMAP\n", cnt);
		printf("Error - no null terminator was found\n");
		return -1;
	}

	//Finish time measuring
	if (gettimeofday(&t2, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}
	// Counting time elapsed
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;
	// Final report
	printf("%d bytes read, %f milliseconds through MMAP\n", cnt, elapsed_millisec);

	//un-ignoring SIGTERM
	if (sigaction(SIGTERM, &old_handler, NULL)!=0){
		printf("Signal handle for ignoring SIGTERM registration failed. %s\n",strerror(errno));
		return errno;
	}

	if (unlink(PATH)<0){
	   printf("Error deleting the file: %s\n", strerror(errno));
	   return errno;
	}

	if (munmap(arr, size)<0){
	   printf("Error un-mmapping the file: %s\n", strerror(errno));
	   return errno;
	}

	// un-linking doesn't close the file, so we still need to do that.
	if(close(file)<0){
	   printf("Error while closing the file: %s\n", strerror(errno));
	   return errno;
	}
	return 0;
}

