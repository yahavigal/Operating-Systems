/*
 * hw3.c
 *
 *  Created on: Dec 27, 2016
 *      Author: osboxes
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>

typedef struct _intnode{
	int value;
	struct _intnode* next;
	struct _intnode* prev;
} intnode;

typedef struct _intlist{
	int size;
	intnode* head;
	intnode* tail;
	pthread_mutex_t lock;
	pthread_cond_t emptyList;
} intlist;

//global vars
intlist globalList;
pthread_cond_t gc_cond;
int reading = 1, writing = 1, collecting = 1, max;

intnode* create_node(int value){
	intnode* node = (intnode*)malloc(sizeof(*node));
	if (node==NULL){
		printf("Error while creating node");
		return NULL;
	}
	node->value = value;
	node->next = NULL;
	node->prev = NULL;
	return node;
}

void destroy_node(intnode* node){
	free(node);
	return;
}

void intlist_init(intlist* list){
	int err;
	pthread_mutexattr_t attr;
	if (list==NULL){
		return;
	}
	if((err = pthread_mutexattr_init(&attr))!=0){
		printf("Error while init the mutex attr: %s\n", strerror(err));
		exit(err);
	}
	if((err = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE))!=0){
		printf("Error while setting the type for the mutex attr: %s\n", strerror(err));
		exit(err);
	}
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	if((err = pthread_mutex_init(&(list->lock), &attr))!=0){
		printf("Error while init the mutex: %s\n", strerror(err));
		exit(err);
	}
	if((err = pthread_cond_init(&(list->emptyList), NULL))!=0){
		printf("Error while init the conditional var: %s\n", strerror(err));
		exit(err);
	}
	return;
}

pthread_mutex_t* intlist_get_mutex(intlist* list){
	if (list==NULL){
		return NULL;
	}
	return &(list->lock);
}

void intlist_push_head(intlist* list, int value){
	int err;
	intnode* node = create_node(value);
	if (list==NULL){
		return;
	}
	if((err = pthread_mutex_lock(&(list->lock)))!=0){
		printf("Error while locking during the push: %s\n", strerror(err));
		exit(err);
	}
	if (list->size == 0){
		list->head = node;
		list->tail = node;
		list->size++;
	}
	else {
		list->head->prev = node;
		node->next = list->head;
		list->head = node;
		(list->size)++;
	}
	if((err = pthread_cond_signal(&(list->emptyList)))!=0){
		printf("Error while signaling during the push: %s\n", strerror(err));
		exit(err);
	}
	if((err = pthread_mutex_unlock(&(list->lock)))!=0){
		printf("Error while unlocking during the push: %s\n", strerror(err));
		exit(err);
	}
	return;
}

int intlist_pop_tail(intlist* list){
	int val, err;
	intnode *tmp1;
	if (list==NULL){
		return -1;
	}
	if((err = pthread_mutex_lock(&(list->lock)))!=0){
		printf("Error while locking during the pop: %s\n", strerror(err));
		exit(err);
	}
	while(list->size == 0){
		if((err = pthread_cond_wait(&(list->emptyList), &(list->lock)))!=0){
			printf("Error while waiting during the pop: %s\n", strerror(err));
			exit(err);
		}
	}

	tmp1 = list->tail;
	val = tmp1->value;

	if (list->size!=1){
		list->tail = tmp1->prev;
		list->tail->next = NULL;
		destroy_node(tmp1);
		list->size--;
	}
	else{
		list->head = NULL;
		list->tail = NULL;
		destroy_node(tmp1);
		list->size--;
	}

	if((err = pthread_mutex_unlock(&(list->lock)))!=0){
		printf("Error while unlocking during the pop: %s\n", strerror(err));
		exit(err);
	}

	return val;
}

void intlist_remove_last_k(intlist* list, int k){
	int h, size, err;
	if (k<0 || list==NULL){
		return;
	}
	if((err = pthread_mutex_lock(&(list->lock)))!=0){
		printf("Error while locking during the k-remove: %s\n", strerror(err));
		exit(err);
	}
	size = list->size;
	if(k>size)
		h = size;
	else
		h = k;
	for (int i=0; i<h; i++){
		intlist_pop_tail(list);
	}
	if((err = pthread_mutex_unlock(&(list->lock)))!=0){
		printf("Error while unlocking during the k-remove: %s\n", strerror(err));
		exit(err);
	}
	return;
}


int intlist_size(intlist* list){
	if (list==NULL){
		return 0;
	}
	return list->size;
}

void intlist_destroy(intlist* list){
	int err;
	intlist_remove_last_k(list,list->size);
	if (list==NULL){
		return;
	}
	if((err = pthread_cond_destroy(&(list->emptyList)))!=0){
		printf("Error while destroying the conditional var: %s\n", strerror(err));
		exit(err);
	}
	if((err = pthread_mutex_destroy(&(list->lock)))!=0){
		printf("Error while destroying the mutex: %s\n", strerror(err));
		exit(err);
	}
	return;
}

void *gcFunc(void *t){
	int err,size;
	if((err = pthread_mutex_lock(intlist_get_mutex(&globalList)))!=0){
		printf("Error while locking during the gc: %s\n", strerror(err));
		exit(err);
	}
	while(collecting){
		while((intlist_size(&globalList)<max)&&(collecting)){
			if((err = pthread_cond_wait(&(gc_cond), intlist_get_mutex(&globalList)))!=0){
				printf("Error while waiting during the gc: %s\n", strerror(err));
				exit(err);
			}
		}
		if(collecting){
			size = ((intlist_size(&globalList))+1)/2;
			intlist_remove_last_k(&globalList, size);
			printf("GC-%d items removed from the list\n", size);
		}
	}
	if((err = pthread_mutex_unlock(intlist_get_mutex(&globalList)))!=0){
		printf("Error while unlocking during the gc: %s\n", strerror(err));
		exit(err);
	}
	pthread_exit(NULL);
}

void *writerFunc(void *t){
	int err;
	time_t h;
	srand((unsigned) time(&h)); // CREDIT https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
	while(writing){
		int i = rand();
		intlist_push_head(&globalList,i);
		if (intlist_size(&globalList)>=max){
			if((err = pthread_cond_signal(&(gc_cond)))!=0){
				printf("Error while signaling during the writing: %s\n", strerror(err));
				exit(err);
			}
		}
	}
	pthread_exit(NULL);
}


void *readerFunc(void *t){
	int err;
	while(reading){
		intlist_pop_tail(&globalList);
	}
	if (intlist_size(&globalList)>=max){
		if((err = pthread_cond_signal(&(gc_cond)))!=0){
			printf("Error while signaling during the reading: %s\n", strerror(err));
			exit(err);
		}
	}
	pthread_exit(NULL);
}


int main(int argc, char* argv[]){
	int wnum, rnum, time, err,val;
	pthread_t gc;
	pthread_attr_t attr;

	// Arrays of writing threads and reading threads
	pthread_t writers[1000], readers[1000];

	// Number of writers, Number of readers, Max nodes in list, Time to sleep
	wnum = strtol(argv[1], NULL, 10);
	rnum = strtol(argv[2], NULL, 10);
	max = strtol(argv[3], NULL, 10);
	time = strtol(argv[4], NULL, 10);

	if (wnum==0){
		printf("The size of the list is 0");
		return 0;
	}

	// Initiating the condition for the GC
	if((err = pthread_cond_init(&gc_cond, NULL))!=0){
		printf("Error while init the conditional var of the gc: %s\n", strerror(err));
		exit(err);
	}

	// Initiating the attribute for being joinable
	if((err = pthread_attr_init(&attr))!=0){
		printf("Error while init the attr for the join: %s\n", strerror(err));
		exit(err);
	}

	if((err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE))!=0){
			printf("Error while setting the attr for the join: %s\n", strerror(err));
			exit(err);
	}

	// the global list
	intlist_init(&globalList);

	// Creating the GC
	pthread_create(&gc, &attr, gcFunc, NULL);

	// Creating the writing threads
	for (int i=0; i<wnum; i++){
		if((err = pthread_create(&writers[i], &attr, writerFunc, NULL))!=0){
			printf("Error while creating writer number %d: %s\n", i, strerror(err));
			exit(err);
		}
	}

	// Creating the reading threads
	for (int i=0; i<rnum; i++){
		if((err = pthread_create(&readers[i], &attr, readerFunc, NULL))!=0){
			printf("Error while creating reader number %d: %s\n", i, strerror(err));
			exit(err);
		}
	}

	// Letting the threads work
	sleep(time);

	// Turn off the readers flag
	reading = 0;

	// Join the readers first so they can close together
	for (int i=0; i<rnum; i++){
		if((err = pthread_join(readers[i], NULL))!=0){
			printf("Error while joining reader number %d: %s\n", i, strerror(err));
			exit(err);
		}
	}

	// Turn off the writers flag
	writing = 0;

	// Join the writers last so the there is no deadlock
	for (int i=0; i<wnum; i++){
		if((err = pthread_join(writers[i], NULL))!=0){
			printf("Error while joining writer number %d: %s\n", i, strerror(err));
			exit(err);
		}
	}

	// Turn off the GC
	collecting = 0;

	if((err = pthread_cond_signal(&gc_cond))!=0){
		printf("Error while signaling during the main: %s\n", strerror(err));
		exit(err);
	}

	if((err = pthread_join(gc, NULL))!=0){
		printf("Error while joining the GC: %s\n", strerror(err));
		exit(err);
	}

	// Printing
	printf("The size of the list is %d and the items within it are: \n", intlist_size(&globalList));

	while(intlist_size(&globalList)!=0){
		val = intlist_pop_tail(&globalList);
		printf("node %d\n",val);
	}

	// Destroying the joinable attr
	if((err = pthread_attr_destroy(&attr))!=0){
		printf("Error while destroying the attr for the join: %s\n", strerror(err));
		exit(err);
	}

	// Destroying the GC's cond_var
	if((err = pthread_cond_destroy(&gc_cond))!=0){
		printf("Error while signaling during the main: %s\n", strerror(err));
		exit(err);
	}

	intlist_destroy(&globalList);

return 0;
}
