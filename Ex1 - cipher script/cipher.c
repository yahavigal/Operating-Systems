/*
 * cipher.c
 *
 *  Created on: 20 бреб 2016
 *      Author: yahav
 */

#define _FILE_OFFSET_BITS 64 //CREDIT: http://stackoverflow.com/questions/1035657/seeking-and-reading-large-files-in-a-linux-c-application

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> // for open flags
#include <unistd.h>
#include <time.h> // for time measurement
#include <sys/time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#define BUFSIZE 4096
#define STRSIZE 1024

int xor_func(struct dirent *dp_in, char* filename_in, int key, char* dir_out, char* filename){
	int file_in,file_out;
	int i,j = 0;
	char filename_out[STRSIZE];
	char buf_file[BUFSIZE], buf_key[BUFSIZE], buf_xor[BUFSIZE];
	ssize_t file_len,key_len;

    if ((file_in = open(filename_in, O_RDWR))<0){
    	printf("Error opening the input file: %s\n", strerror(errno));
    	return -1;
    }
    // full path to out file
    sprintf(filename_out, "%s/%s", dir_out, filename);

	if ((file_out = creat(filename_out, O_RDWR | O_TRUNC | 0777))<0){
		printf("Error creating the out file: %s\n", strerror(errno));
		return -1;
	}

    // read 4096 characters each time
    while(1){
    	file_len = read(file_in, buf_file, BUFSIZE);
    	key_len = read(key, buf_key, BUFSIZE);
    	if (file_len < 0 || key_len < 0) {
    		printf("Error reading from file: %s\n", strerror(errno));
    		return -1;
    	}
    	if (file_len == 0){
    		break;
    	}

    	// bitwise xor
		for (i=0; i<file_len; i++){
			buf_xor[i] = (char)(buf_file[i] ^ buf_key[j]);
			// if key is shorter than file, repeat key
			if ((file_len>key_len) && (j>=key_len)){
				if(lseek(key,0,SEEK_SET)<0){
					printf("Error returning to the start of the key file: %s\n", strerror(errno));
					return -1;
				}
				j=0;
			}
			else {
				j++;
			}
		}
		// write to out file
		if (write(file_out, buf_xor, file_len) < 0) {
			printf("Error writing to out file: %s\n", strerror(errno));
			return -1;
		}
		if(lseek(key,0,SEEK_SET)<0){
			printf("Error returning to the start of the key file: %s\n", strerror(errno));
			return -1;
		}
    }
	// close files
	if (close(file_in)<0){
		printf("Error closing the input file: %s\n", strerror(errno));
		return -1;
	}
	if (close(file_out)<0){
		printf("Error closing the output file: %s\n", strerror(errno));
		return -1;
	}
	return 1;
}

int main(int argc, char* argv[]) {
    int key;
	struct dirent *dp_in;
    DIR *dfd_in,*dfd_out;
    char filename_in[STRSIZE],*dir_in,*dir_out,*keypath;
    struct stat statbuf;
    dir_in = argv[1];
    keypath = argv[2];
    dir_out = argv[3];

    // open directory in stream
    if ((dfd_in = opendir(dir_in)) == NULL) {
		printf("Error opening directory: %s\n", strerror(errno));
		return -1; 
	}
    // open directory out stream
     if ((dfd_out = opendir(dir_out)) == NULL) {
		printf("Error opening directory: %s\n", strerror(errno));
		return -1; 
	}
     if ((key = open(keypath, O_RDWR))<0){
		printf("Error opening the key file: %s\n", strerror(errno));
		return -1;
    }
    while ((dp_in = readdir(dfd_in)) != NULL) {
        // full path to file
        sprintf(filename_in, "%s/%s", dir_in,dp_in->d_name) ;

        // call stat to get file metadata
        if (stat(filename_in,&statbuf ) < 0) {
			printf("Error getting stat of file: %s\n", strerror(errno));
			return -1; 
		}
    	// skip directories
    	if (!S_ISDIR(statbuf.st_mode)) {
    		if (xor_func(dp_in, filename_in, key, dir_out,dp_in->d_name)<0){
    			return -1;
    		}
       	}
    }
    //close everything
    if (close(key)<0){
		printf("Error closing the key file: %s\n", strerror(errno));
		return -1;
    }
    if (closedir(dfd_in)<0){
		printf("Error closing the input directory: %s\n", strerror(errno));
		return -1;
    }
    if (closedir(dfd_out)<0){
		printf("Error closing the output directory: %s\n", strerror(errno));
		return -1;
    }
    return 0;
}

