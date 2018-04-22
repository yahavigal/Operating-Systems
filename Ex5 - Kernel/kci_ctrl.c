

/*
 *  kci_ctrl.c
 *  Created on: 28 Jan 2017
 *  Author: osboxes
 */

#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "kci.h"

#define _BSD_SOURCE

int initmod(char* path){
	int rc, fd, nd;
	dev_t dev;

	if ((fd = open(path, 0))<0){
		printf("Error : Could not open the path : %s\n", strerror(errno));
		return (errno);
	}

	if ((rc = syscall(__NR_finit_module, fd, "", 0))<0){
		printf("Error : Could not init module : %s\n", strerror(errno));
		return (errno);
	}

	dev = makedev(MAJOR_NUM, MINOR_NUM);

	if ((nd = mknod(DEVICE_FILE_NAME,S_IFCHR,dev))<0){
		printf("Error : Could not create filesystem node : %s\n", strerror(errno));
		return (errno);
	}

	close(fd);
	return 0;
}

int pidset(int pid){
	int ret, fd;
	fd = open(DEVICE_FILE_NAME,O_RDONLY);
	if ((ret = ioctl(fd,IOCTL_SET_PID,pid))<0){
		printf("Error : Could not set PID : %s\n", strerror(errno));
		return (errno);
	}

	close(fd);
	return 0;
}

int fdset(int fdnum){
	int ret, fd;
	fd = open(DEVICE_FILE_NAME,O_RDONLY);
	if ((ret = ioctl(fd,IOCTL_SET_FD,fdnum))<0){
		printf("Error : Could not set FD : %s\n", strerror(errno));
		return (errno);
	}

	close(fd);
	return 0;
}

int start(){
	int ret, fd;
	fd = open(DEVICE_FILE_NAME,O_RDONLY);
	if ((ret = ioctl(fd,IOCTL_CIPHER,1))<0){
		printf("Error : Could not start module : %s\n", strerror(errno));
		return (errno);
	}

	close(fd);
	return 0;
}

int stop(){
	int ret, fd;
	fd = open(DEVICE_FILE_NAME,O_RDONLY);
	if ((ret = ioctl(fd,IOCTL_CIPHER,0))<0){
		printf("Error : Could not stop module : %s\n", strerror(errno));
		return (errno);
	}

	close(fd);
	return 0;
}

int rem(){
	int oldLog, newLog, notwritten, numwritten, written, rc;
	char buff[BUFSIZE] = {0}; 	oldLog = open(OLDLOG,O_RDONLY);
	if (oldLog <0){
		printf("Error : Could not open old log file: %s\n", strerror(errno));
		return (errno);
	}
	newLog = open(NEWLOG,O_CREAT|O_WRONLY|O_TRUNC,0777);
	if (newLog <0){
		printf("Error : Could not open or create new log file: %s\n", strerror(errno));
		return (errno);
	}

	while((notwritten = read(oldLog, buff, sizeof(buff)))>0){ // set number of bytes written to the new log file to 0
		numwritten = 0;     	// iterate writing to the new log
		while (numwritten < notwritten) {
			if ((written = write(newLog, buff + numwritten, notwritten - numwritten))<0){
				printf("error writing to new log file: %s\n", strerror(errno));
				return (errno);
			}
			numwritten += written;
		}
	}
	if (notwritten < 0){
		printf("Error occurred while reading from the old log file. %s \n", strerror(errno));
		return (errno);
	}
	close(oldLog);
	close(newLog);
	if((rc = syscall(__NR_delete_module, DEVICE_RANGE_NAME, O_NONBLOCK))<0){
		printf("Error : Could not init module : %s\n", strerror(errno));
		return (errno);
	}
	if((unlink(DEVICE_FILE_NAME))<0){
		printf("Error : Could not unlink device : %s\n", strerror(errno));
		return (errno);
	}

	return 0;
}

int main(int argc, char* argv[]){
	char* ptr;
	if (strcmp(argv[1], "-init")==0){
		initmod(argv[2]);
	}
	else if (strcmp(argv[1], "-pid")==0){
		pidset(strtol(argv[2], &ptr, 10));
	}
	else if (strcmp(argv[1], "-fd")==0){
		fdset(strtol(argv[2], &ptr, 10));
	}
	else if (strcmp(argv[1], "-start")==0){
		start();
	}
	else if (strcmp(argv[1], "-stop")==0){
		stop();
	}
	else if (strcmp(argv[1], "-rm")==0){
		rem();
	}
}



