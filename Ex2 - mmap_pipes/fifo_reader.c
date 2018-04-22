/*
 * fifo_reader.c
 *
 *  Created on: 8 Dec 2016
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

#define PATH "/tmp/osfifo"
#define PAGE 4000

int main(){
	int file, i, bRead, cnt=0, total=0;
	char arr[PAGE];
	struct sigaction ignr_sigint, old_handler;
	struct timeval t1, t2;
	double elapsed_millisec;
	sleep(1);

	// Assign pointer to our handler function
	ignr_sigint.sa_handler = SIG_IGN;

	// Remove any special flag
	ignr_sigint.sa_flags = 0;

	//ignoring SIGINT
	if (sigaction(SIGINT, &ignr_sigint, &old_handler)!=0){
		printf("Signal handle for ignoring SIGINT registration failed. %s\n",strerror(errno));
		return errno;
	}

    //open the created file for writing
    if((file = open(PATH,O_RDONLY))<0){
    	printf("Error while opening the file. %s\n",strerror(errno));
    	return errno;
    }

	// Start time measuring
	if(gettimeofday(&t1, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}

	while ((bRead = read(file, arr, PAGE))>0){
		for (i=0; i<bRead; i++){
			if (arr[cnt]=='a'){
				cnt++;
			}
			total+=cnt;
			cnt = 0;
		}
	}

	if(bRead<0){
		printf("Error reading from pipe: %s\n", strerror(errno));
		return errno;
	}

	// Finish time measuring
	if (gettimeofday(&t2, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}
	// Counting time elapsed
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;
	// Final report
	printf("%d bytes read, %f milliseconds through FIFO\n", total, elapsed_millisec);

	//un-ignoring SIGINT
	if (sigaction(SIGINT, &old_handler, NULL)!=0){
		printf("Signal handle for ignoring SIGINT registration failed. %s\n",strerror(errno));
		return errno;
	}

	if(close(file)<0){
	   printf("Error while closing the file: %s\n", strerror(errno));
	   return errno;
	}
    return 0;
}
