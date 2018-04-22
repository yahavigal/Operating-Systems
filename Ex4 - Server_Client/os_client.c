/*
 * client.c
 *
 *  Created on: 15 Jan 2017
 *      Author: yahav
 */


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <sys/mman.h>

#define OK 0
#define EXIT -1
#define BUFSIZE 1024

int main(int argc, char *argv[]){
    int sockfd = 0, port, in, out;
    int readIn = 0, sentToServer = 0, sentByNow = 0, readFromServer = 0;
    int diff = 0, writeOut = 0, written = 0, writtenByNow = 0;
    char recvBuff[BUFSIZE] = {0}, sendBuff[BUFSIZE] = {0}, *ip;
    struct sockaddr_in serv_addr = {0};

	// ip as a string, port as positive short, input (from user) and output (from server) files
	ip = argv[1];
	port = strtol(argv[2], NULL, 10);
	in = open(argv[3],O_RDONLY);// IN must exist
	out = open(argv[4],O_CREAT|O_WRONLY|O_TRUNC,0777);// if OUT doesn't exist - create it, if it does - truncate it

	if (ip == NULL){
		printf ("Error in IP\n");
		return EXIT;
	}

	if (in <0){
        printf("Error : Could not open input file: %s\n", strerror(errno));
        return (errno);
	}

	if (out <0){
        printf("Error : Could not open or create output file: %s\n", strerror(errno));
        return (errno);
	}

	// configuring the socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error : Could not create socket: %s\n", strerror(errno));
        return (errno);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
       printf("Error : Connect Failed. %s \n", strerror(errno));
       return (errno);
    }

    // reading from IN file
    while((readIn = read(in, sendBuff, sizeof(sendBuff)))>0){
    	// writing to the server
    	while (readIn > 0){
        	if ((sentToServer = write(sockfd, sendBuff + sentByNow, readIn)) < 0){// nsent = written in last write() call
        	    printf("Error occurred while writing to the server. %s \n", strerror(errno));
        		return (errno);
        	}
        	// reading from server file and writing to the OUT
        	readFromServer = 0;
        	diff = sentToServer;
        	while(diff>0){
        		readFromServer = read(sockfd, recvBuff, diff);
        		diff -= readFromServer;
        		writeOut += readFromServer;
        	}
        	if (readFromServer < 0){
        		printf("Error occurred while reading from the server. %s \n", strerror(errno));
        		return (errno);
        	}

        	while (writeOut > 0){
            	if ((written = write(out, recvBuff + writtenByNow, writeOut)) < 0){// nsent = written in last write() call
            		printf("Error occurred while writing to the OUT file. %s \n", strerror(errno));
            	    return (errno);
            	}
        		writeOut -= written;
        		writtenByNow += written;
        	}
        	writtenByNow = 0;
    		sentByNow  += sentToServer;
    		readIn -= sentToServer;
    	}
    	sentByNow = 0;
    }
	if (readIn < 0){
		printf("Error occurred while reading from IN file. %s \n", strerror(errno));
		return (errno);
	}
	close(in);
	close(out);
    close(sockfd);
    return OK;
}
