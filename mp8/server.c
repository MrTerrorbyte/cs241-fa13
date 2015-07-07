/** @file server.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "queue.h"
#include "libhttp.h"
#include "libdictionary.h"

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

pthread_t threadsArray[1024];
int numThreads;
int socketsArray[1024];
int numSockets;

/**
 * Processes the request line of the HTTP header.
 * 
 * @param request The request line of the HTTP header.  This should be
 *                the first line of an HTTP request header and must
 *                NOT include the HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
		return NULL;

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}

void * worker(void * sock_fd){
	int keepAlive= 1;
	int sock = *((int *) sock_fd);
	http_t http;
	while(keepAlive){
		if(http_read(&http, sock) == -1){
			printf("http_read failed\n");
			close(sock);
			//pthread_exit(NULL);
			return NULL;
		}
		
		const char * status = http_get_status(&http);
		char * fileName = process_http_header_request(status);
		int response;
		int contentLen;
		const char * responseString = NULL;
		const char * content = NULL;
		char * content200 = NULL;
		const char * contentType = NULL;
		char * connection = NULL;
		char * retFileName = NULL;
		if(!fileName){
			response = 501;
			contentLen = strlen(HTTP_501_CONTENT);
			responseString = HTTP_501_STRING;
			content = HTTP_501_CONTENT;
		}
		else if(fileName){
			if(strcmp(fileName, "/") == 0){
				fileName = realloc(fileName, strlen(fileName) + 11);
				strcat(fileName, "index.html");
			}
			retFileName = malloc(strlen(fileName) + strlen("web") + 1);
			strcpy(retFileName, "web");
			strcat(retFileName, fileName);
			retFileName[strlen(retFileName)] = '\0';
			free(fileName);
			FILE * file = fopen(retFileName, "r");
			if(!file){
				response = 404;
				contentLen = strlen(HTTP_404_CONTENT);
				responseString = HTTP_404_STRING;
				content = HTTP_404_CONTENT;
			}
			else{
				response = 200;
				struct stat buffer;
				stat(retFileName, &buffer);
				contentLen = buffer.st_size;
				content200 = malloc(contentLen);
				fread(content200, contentLen, 1, file);
				responseString = HTTP_200_STRING;
				fclose(file);
			}
		}
		if(response == 501 || response == 404){
			contentType = "text/html";
		}
		else{
			char * type = strrchr(retFileName, '.');
			if(strcmp(type, ".html") == 0)
				contentType = "text/html";
			else if(strcmp(type, ".css") == 0)
				contentType = "text/css";
			else if(strcmp(type, ".jpg") == 0)
				contentType  = "image/jpeg";
			else if(strcmp(type, ".png") == 0)
				contentType = "image/png";
			else contentType = "text/plain";
		}

		char * retResponse = malloc(contentLen + 1024);
		char line[512];
		sprintf(line, "HTTP/1.1 %d %s\r\n", response, responseString);
		strcpy(retResponse, line);

		sprintf(line, "Content-Type: %s\r\n", contentType);
		strcat(retResponse, line);

		sprintf(line, "Content-Length: %d\r\n", contentLen);
		strcat(retResponse, line);

		const char * retCon = http_get_header(&http, "Connection");
		if(retCon != NULL && strcasecmp(retCon, "Keep-Alive") == 0)
			connection = "Keep-Alive";
		else connection = "close";

		sprintf(line, "Connection: %s\r\n", connection);
		strcat(retResponse, line);
		strcat(retResponse, "\r\n");

		int responseLen = strlen(retResponse);
		if(response == 200)
			memcpy(retResponse+responseLen, content200, contentLen);
		else memcpy(retResponse+responseLen, content, contentLen);

		responseLen += contentLen;

		send(sock, retResponse, responseLen, 0);

		if(strcmp(connection, "Keep-Alive") != 0)
			keepAlive= 0;
		
		if(retFileName)
			free(retFileName);
		
		free(retResponse);
		if(content200)
			free(content200);

		http_free(&http);
	}

	close(sock);
	return NULL;
}

void handler(int sig){
	int i=0;
	for( ; i < numThreads; i++){
		pthread_join(threadsArray[i], NULL);
	}
	for(i = 0; i < numSockets; i++){
		shutdown(socketsArray[i], 2);
	}
	printf("caught signal\n");
	exit(sig);
}

int main(int argc, char **argv)
{
	signal(SIGINT, handler);
	if(argc != 2){
		fprintf(stderr, "wrong # of arguments\n");
		exit(1);
	}
	numThreads = 0;
	numSockets = 0;
	queue_t q;
	queue_init(&q);
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct addrinfo hints, *result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags= AI_PASSIVE;
	int s = getaddrinfo(NULL, argv[1], &hints, &result); //argv[1] = port#
	if(s != 0){
		fprintf(stderr, "error in getaddrinfo()\n");
		exit(1);
	}
	if(bind(sock_fd, result->ai_addr, result->ai_addrlen) != 0){
		fprintf(stderr, "error in bind()\n");
		exit(1);
	}
	if(listen(sock_fd, 10) != 0){
		fprintf(stderr, "error in listen()");
		exit(1);
	}
	freeaddrinfo(result);	
	while(1){
		int c_sock = accept(sock_fd, NULL, NULL);
		numSockets++;
		numThreads++;
		socketsArray[numSockets-1] = c_sock;
		pthread_create(&threadsArray[numThreads -1], NULL, &worker, &c_sock);
	}

	return 0;
}
