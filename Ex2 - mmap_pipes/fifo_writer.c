/*
 * fifo_writer.c
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
#define DEC 10

int cnt=0, file;
struct timeval t1, t2;
double elapsed_millisec;

void my_sigpipe_handler (int signum){

	// Finish time measuring
	if (gettimeofday(&t2, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		exit(errno);
	}
	// Counting time elapsed
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;
	// Final report
	printf("%d bytes written, %f milliseconds through FIFO\n", cnt, elapsed_millisec);

	if (unlink(PATH)<0){
	   printf("Error deleting the file: %s\n", strerror(errno));
	   exit(errno);
	}

	// un-linking doesn't close the file, so we still need to do that.
	if(close(file)<0){
	   printf("Error while closing the file: %s\n", strerror(errno));
	   exit(errno);
	}

	printf("Exit due to SIGPIPE\n");
	exit(-1);
}

int main(int argc, char* argv[]){
    int bytesLeftToW, i, bWritten;
	struct sigaction sig_handler, ignr_sigint, old_handler;
    char arr[PAGE], *ptr;
	bytesLeftToW = strtol(argv[1], &ptr, DEC);

    for(i=0; i<PAGE; i++){
    	arr[i] = 'a';
    }

	// Assign pointer to our handler function
	sig_handler.sa_handler = my_sigpipe_handler;
	ignr_sigint.sa_handler = SIG_IGN;

	// Remove any special flag
	sig_handler.sa_flags = 0;
	ignr_sigint.sa_flags = 0;

	//ignoring SIGINT
	if (sigaction(SIGINT, &ignr_sigint, &old_handler)!=0){
		printf("Signal handle for ignoring SIGINT registration failed. %s\n",strerror(errno));
		return errno;
	}

	// Register the handler
	if (sigaction(SIGPIPE, &sig_handler, NULL)!=0){
		printf("Signal handle registration failed. %s\n",strerror(errno));
		return errno;
	}

    //create the named pipe
    if(mkfifo(PATH, 0600)<0){
    	if (errno == EEXIST){
    		chmod (PATH, 0600);
    	}
    	else{
			printf("Error making the fifo: %s\n", strerror(errno));
			return errno;
    	}
    }

    //open the created file for writing
    if((file = open(PATH,O_WRONLY))<0){
    	printf("Error while opening the file. %s\n",strerror(errno));
    	return errno;
    }

	// Start time measuring
	if(gettimeofday(&t1, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}

	while (bytesLeftToW>PAGE){
		bWritten = write(file, arr, PAGE);
		if(bWritten<0){
			printf("Error writing to pipe: %s\n", strerror(errno));
			return errno;
		}
		bytesLeftToW-=bWritten;
		cnt+=bWritten;
	}

	if((bWritten = write(file, arr, bytesLeftToW))<0){
		printf("Error writing to pipe: %s\n", strerror(errno));
		return errno;
	}
	cnt+=bWritten;

	// Finish time measuring
	if (gettimeofday(&t2, NULL)<0){
		printf("Error measuring the time: %s\n", strerror(errno));
		return errno;
	}
	// Counting time elapsed
	elapsed_millisec = (t2.tv_sec - t1.tv_sec) * 1000.0;
	elapsed_millisec += (t2.tv_usec - t1.tv_usec) / 1000.0;
	// Final report
	printf("%d bytes written, %f milliseconds through FIFO\n", cnt, elapsed_millisec);

	//un-ignoring SIGINT
	if (sigaction(SIGINT, &old_handler, NULL)!=0){
		printf("Signal handle for ignoring SIGINT registration failed. %s\n",strerror(errno));
		return errno;
	}

	if (unlink(PATH)<0){
	   printf("Error deleting the file: %s\n", strerror(errno));
	   return errno;
	}

	// un-linking doesn't close the file, so we still need to do that.
	if(close(file)<0){
	   printf("Error while closing the file: %s\n", strerror(errno));
	   return errno;
	}
    return 0;
}
