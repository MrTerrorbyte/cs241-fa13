/** @file libmapreduce.c */
/* 
 * CS 241
 * The University of Illinois
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>

#include "libmapreduce.h"
#include "libds/libds.h"



static const int BUFFER_SIZE = 2048;  /**< Size of the buffer used by read_from_fd(). */
pthread_t thread;
int numValues;
pid_t * pidArray;
int ** fds;
/**
 * Adds the key-value pair to the mapreduce data structure.  This may
 * require a reduce() operation.
 *
 * @param key
 *    The key of the key-value pair.  The key has been malloc()'d by
 *    read_from_fd() and must be free()'d by you at some point.
 * @param value
 *    The value of the key-value pair.  The value has been malloc()'d
 *    by read_from_fd() and must be free()'d by you at some point.
 * @param mr
 *    The pass-through mapreduce data structure (from read_from_fd()).
 */
static void process_key_value(const char *key, const char *value, mapreduce_t *mr)
{
	unsigned long rev;
	const char * oldVal= datastore_get(mr->data, key, &rev);
	if(!oldVal){
		datastore_put(mr->data, key, value);
	}
	else{
		const char * newVal= mr->myreduce(oldVal, value);
		datastore_update(mr->data, key, newVal, rev);
		free((char *)newVal);
	}
	if(oldVal){
		free((char *)oldVal);
	}
	free((char *)value);
	free((char *)key);
}


/**
 * Helper function.  Reads up to BUFFER_SIZE from a file descriptor into a
 * buffer and calls process_key_value() when for each and every key-value
 * pair that is read from the file descriptor.
 *
 * Each key-value must be in a "Key: Value" format, identical to MP1, and
 * each pair must be terminated by a newline ('\n').
 *
 * Each unique file descriptor must have a unique buffer and the buffer
 * must be of size (BUFFER_SIZE + 1).  Therefore, if you have two
 * unique file descriptors, you must have two buffers that each have
 * been malloc()'d to size (BUFFER_SIZE + 1).
 *
 * Note that read_from_fd() makes a read() call and will block if the
 * fd does not have data ready to be read.  This function is complete
 * and does not need to be modified as part of this MP.
 *
 * @param fd
 *    File descriptor to read from.
 * @param buffer
 *    A unique buffer associated with the fd.  This buffer may have
 *    a partial key-value pair between calls to read_from_fd() and
 *    must not be modified outside the context of read_from_fd().
 * @param mr
 *    Pass-through mapreduce_t structure (to process_key_value()).
 *
 * @retval 1
 *    Data was available and was read successfully.
 * @retval 0
 *    The file descriptor fd has been closed, no more data to read.
 * @retval -1
 *    The call to read() produced an error.
 */
static int read_from_fd(int fd, char *buffer, mapreduce_t *mr)
{
	/* Find the end of the string. */
	int offset = strlen(buffer);

	/* Read bytes from the underlying stream. */
	int bytes_read = read(fd, buffer + offset, BUFFER_SIZE - offset);
	if (bytes_read == 0)
		return 0;
	else if(bytes_read < 0)
	{
		fprintf(stderr, "error in read.\n");
		return -1;
	}

	buffer[offset + bytes_read] = '\0';

	/* Loop through each "key: value\n" line from the fd. */
	char *line;
	while ((line = strstr(buffer, "\n")) != NULL)
	{
		*line = '\0';

		/* Find the key/value split. */
		char *split = strstr(buffer, ": ");
		if (split == NULL)
			continue;

		/* Allocate and assign memory */
		char *key = malloc((split - buffer + 1) * sizeof(char));
		char *value = malloc((strlen(split) - 2 + 1) * sizeof(char));

		strncpy(key, buffer, split - buffer);
		key[split - buffer] = '\0';

		strcpy(value, split + 2);

		/* Process the key/value. */
		process_key_value(key, value, mr);

		/* Shift the contents of the buffer to remove the space used by the processed line. */
		memmove(buffer, line + 1, BUFFER_SIZE - ((line + 1) - buffer));
		buffer[BUFFER_SIZE - ((line + 1) - buffer)] = '\0';
		//printf("buffer= %s\n", buffer);
	}

	return 1;
}


/**
 * Initialize the mapreduce data structure, given a map and a reduce
 * function pointer.
 */
void mapreduce_init(mapreduce_t *mr, 
                    void (*mymap)(int, const char *), 
                    const char *(*myreduce)(const char *, const char *))
{
	mr->data= malloc(sizeof(datastore_t));
	datastore_init(mr->data);
	mr->mymap= mymap;
	mr->myreduce= myreduce;
	mr->epoll_fd= epoll_create(10);
}

int getIndex(mapreduce_t * mr, int fd){
	int i=0;
	for( ; i < numValues; i++){
		if(mr->fdBufferArr[i]->fd == fd){
			return mr->fdBufferArr[i]->index;
		}
	}
	return -1;
}

void * worker(void * mr0){
	char ** buffers= malloc(sizeof(char *) * numValues);
	mapreduce_t * mr= (mapreduce_t *)mr0;
	int i= 0;
	for( ; i < numValues; i++){
		buffers[i]= malloc(BUFFER_SIZE+1);
		buffers[i][0]= '\0';
	}
	int n= numValues;
	while(n){
		i=0;
		struct epoll_event ev;
		epoll_wait(mr->epoll_fd, &ev, 1, -1);
		int index= getIndex(mr, ev.data.fd);
		if(index == -1)
			printf("error at getting index\n");
		int bytes= read_from_fd(ev.data.fd, buffers[index], mr);
        
		if(bytes == 0){
			n--;
			epoll_ctl(mr->epoll_fd,EPOLL_CTL_DEL,ev.data.fd,NULL);
		}
		else if(bytes == -1){
			printf("error\n");
		}
	}
	for(i=0; i < numValues; i++)
		free(buffers[i]);
	free(buffers);
	return NULL;
}

/**
 * Starts the map() processes for each value in the values array.
 * (See the MP description for full details.)
 */
void mapreduce_map_all(mapreduce_t *mr, const char **values)
{
	numValues= 0;
	int i= 0, j=0;
	for( ; values[i]!=NULL; i++){
		numValues++;
	}

	mr->fdBufferArr= malloc(sizeof(fdBuffer *) * numValues);

	pidArray= malloc(sizeof(pid_t) * numValues);
	fds= malloc(numValues * sizeof(int *));
	pid_t child;
	//mr->epoll_fd= epoll_create(10);
	struct epoll_event event[numValues];
	for(i=0; i < numValues; i++){
		fds[i]= malloc(2 * sizeof(int));
		pipe(fds[i]);
		child= fork();

		if(child == 0){ //child
			for( ; j <= i; j++){
				close(fds[j][0]); //close all read
			}
			mr->mymap(fds[i][1], values[i]);
			exit(0);
		}
		else if(child > 0){ //parent
			pidArray[i]= child;
			close(fds[i][1]); //close write
		}
		bzero(&event[i], sizeof(struct epoll_event));
		event[i].events= EPOLLIN;
		event[i].data.fd= fds[i][0]; //read fd
		mr->fdBufferArr[i]= malloc(sizeof(fdBuffer));
		mr->fdBufferArr[i]->fd= fds[i][0];
		mr->fdBufferArr[i]->index= i;
		epoll_ctl(mr->epoll_fd, EPOLL_CTL_ADD, fds[i][0], &event[i]);
	}
	pthread_create(&thread, NULL, &worker, (void *)mr);
}



/**
 * Blocks until all the reduce() operations have been completed.
 * (See the MP description for full details.)
 */
void mapreduce_reduce_all(mapreduce_t *mr)
{
	int i= 0;
	int status;
	for( ; i < numValues; i++){
		waitpid(pidArray[i], &status, 0);
	}
	pthread_join(thread, NULL);
}


/**
 * Gets the current value for a key.
 * (See the MP description for full details.)
 */
const char *mapreduce_get_value(mapreduce_t *mr, const char *result_key)
{
	return datastore_get(mr->data, result_key, NULL);
}


/**
 * Destroys the mapreduce data structure.
 */
void mapreduce_destroy(mapreduce_t *mr)
{
	datastore_destroy(mr->data);
	free(mr->data);
	int i=0;
	for( ; i < numValues; i++){
		free(fds[i]);
		free(mr->fdBufferArr[i]);
	}
	free(fds);
	free(pidArray);
	free(mr->fdBufferArr);
}
