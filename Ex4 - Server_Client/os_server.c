/*
 * server.c
 *
 *  Created on: 15 Jan 2017
 *      Author: yahav
 */

#define _FILE_OFFSET_BITS 64
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
#include <sys/stat.h>
#include <dirent.h>

#define OK 0
#define EXIT -1
#define PATH "/dev/urandom"
#define BUFSIZE 1024

int listenfd = -1;
int connfd = -1;
int key = -1;
pid_t pid = -1;

void my_sig_handler (int signum){
	if (pid==0){
		if(key!=-1){
			close(key);
			key = -1;
		}
		if(connfd!=-1){
			close(connfd);
			connfd = -1;
		}
		if(listenfd!=-1){
			close(listenfd);
			listenfd = -1;
		}
	}
	else{
		if(connfd!=-1){
			close(connfd);
			connfd = -1;
		}
		if(listenfd!=-1){
			close(listenfd);
			listenfd = -1;
		}
	}
	exit(-1);
}

int main(int argc, char *argv[]){
    int port, keylen, path, numkey, num;
    int totalsent = 0, notwritten, nsent;
    struct sockaddr_in serv_addr = {0};
    char recvBuff[BUFSIZE] = {0}, sendBuff[BUFSIZE] = {0}, keyBuff[BUFSIZE] = {0}, *keypath;
    struct sigaction sig_handler;
    struct stat sizecheck;

    port = strtol(argv[1], NULL, 10); // Assuming a positive short
    keypath = argv[2];
    if (argc == 3){
    	stat(keypath, &sizecheck);
    	if (sizecheck.st_size<=0){
    		printf("KEYLEN was not provided and KEYPATH does not exit or is empty\n");
    		return EXIT;
    	}
    }
    if (argc == 4){
    keylen = strtol(argv[3], NULL, 10); // Assuming a positive integer
		if (keylen<=0){
			printf("Error - keylen is not positive\n");
			return EXIT;
		}
		key = open(keypath,O_CREAT|O_WRONLY|O_TRUNC,0777); // create a new file or truncate an existing file
		if (key <0){
			printf("Error : Could not open or create key file: %s\n", strerror(errno));
			return (errno);
		}
		path = open(PATH,O_RDONLY); // open "/dev/urandom"
		if (path <0){
			printf("Error : Could not open /dev/urandom: %s\n", strerror(errno));
			return (errno);
		}

		// reading from PATH file and writing to the key file

		while(keylen>sizeof(keyBuff)){
			if((notwritten = read(path, keyBuff, sizeof(keyBuff)))<0){
				printf("Error occurred while reading from /dev/urandom. %s \n", strerror(errno));
				return (errno);
			}
			while (notwritten > 0){
				if ((nsent = write(key, keyBuff + totalsent, notwritten)) < 0){
					printf("Error occurred while writing to the key file. %s \n", strerror(errno));
					return (errno);
				}
				totalsent  += nsent;
				notwritten -= nsent;
				keylen -= nsent;
			}
			totalsent = 0;
		}
		while(keylen!=0){
			if((notwritten = read(path, keyBuff, keylen))<0){
				printf("Error occurred while reading from /dev/urandom for last time. %s \n", strerror(errno));
				return (errno);
			}
			while (notwritten > 0){
				if ((nsent = write(key, keyBuff + totalsent, notwritten)) < 0){
					printf("Error occurred while writing to the key file for last time. %s \n", strerror(errno));
					return (errno);
				}
				totalsent  += nsent;
				notwritten -= nsent;
				keylen -= nsent;
			}
			totalsent = 0;
		}
		close(key);
		key = -1;
		close(path);
		totalsent = 0;
    }
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
    serv_addr.sin_port = htons(port);

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
       printf("Error : Bind Failed. %s \n", strerror(errno));
       return (errno);
    }

    if(listen(listenfd, 10)){
       printf("Error : Listen Failed. %s \n", strerror(errno));
       return (errno);
    }

    // Assign pointer to our handler function
    sig_handler.sa_handler = my_sig_handler;
	// Remove any special flag
	sig_handler.sa_flags = 0;
	// Register the handler
	if (sigaction(SIGINT, &sig_handler, NULL)!=0){
		printf("Signal handle registration failed. %s\n",strerror(errno));
		return (errno);
	}

    while(1){
        // accepting connection
        if((connfd = accept(listenfd, NULL, NULL))<0){
			printf("Error : Accept Failed. %s \n", strerror(errno));
			return (errno);
		}

        pid = fork();
        if (pid<0){
        	printf("Error while forking %s \n", strerror(errno));
        	return (errno);
        }
        if(pid == 0){ // son process
        	if((key = open(keypath,O_RDONLY))<0){
				printf("Error : Could not open key file in son's process: %s\n", strerror(errno));
				return (errno);
			}
			// receiving from client, encrypting and sending back
			while((notwritten = read(connfd, recvBuff, sizeof(recvBuff)))>0){
				// set number of bytes read from the key file to 0
				numkey = 0;
		    	// iterate reading from key until reaching numwritten bytes
		    	while (numkey < notwritten) {
		    		if ((num = read(key, keyBuff + numkey, notwritten - numkey))<0){
		    			printf("error read() key: %s\n", strerror(errno));
		    			return (errno);
		    		}
		    		// reached end of key, reset and read from start
		    		else if (num == 0) {
		    			// we must check lseek() - it's a system call!
		    			if (lseek(key, SEEK_SET, 0) < 0) {
		    				printf("error lseek() key: %s\n", strerror(errno));
		    				return (errno);
		    			}
		    		}
		    		// success - increase our counter of bytes read from key
		    		else {
		    			numkey += num;
		    		}
		    	}
		    	// now we have 'numsrc' bytes - from both input and key file
		    	// perform encryption operation
		    	for (int i = 0; i < notwritten; ++i){
		    		sendBuff[i] = recvBuff[i] ^ keyBuff[i];
		    	}
		    	while (notwritten > 0){
		        	if ((nsent = write(connfd, sendBuff + totalsent, notwritten)) < 0){// nsent = written in last write() call
		        		printf("Error occurred while writing to the client. %s \n", strerror(errno));
		        	    return (errno);
		        	}
		    		totalsent  += nsent;
		    		notwritten -= nsent;

		    	}
		    	totalsent = 0;
			}
		    if (notwritten < 0){
				printf("Error occurred while reading from the client. %s \n", strerror(errno));
				return (errno);
			}

			if(key!=-1){
				close(key);
				key = -1;
			}
			if(connfd!=-1){
				close(connfd);
				connfd = -1;
			}
			if(listenfd!=-1){
				close(listenfd);
				listenfd = -1;
			}
	        return EXIT;
        }
    }
}
