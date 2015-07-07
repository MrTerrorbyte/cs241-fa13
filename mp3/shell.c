/** @file shell.c */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "log.h"


log_t  Log;

/**
 * Starting point for shell.
 */
int main() {
	log_init(&Log);
	char cwd[1024];
	int pid;
	size_t size= 0;
	while(1){
		pid= getpid();
		getcwd(cwd, 1024);
		printf("(pid=%d)%s$ ", pid, cwd);
		char * line= NULL;
		getline(&line, &size, stdin);
		line[strlen(line)-1]= '\0';
		if(strstr(line, "cd") == line){
			const char * path= line+3;
			int change= chdir(path);
			if(change==0){
				int pid= getpid();
				printf("Command executed by pid=%d\n", pid);
				log_push(&Log, line);
				continue;
			}
			else{
				printf("%s: No such file or directory\n", path);
				log_push(&Log, line);
				continue;
			}
		}

		if(strstr(line, "exit") == line){
			int pid= getpid();
			printf("Command executed by pid=%d\n", pid);
			log_push(&Log, line);
			log_destroy(&Log);
			
			exit(1);
		}

		if(strstr(line, "!#")){
			log_entry * curr= Log.head;
			while(curr){
				printf("%s\n", curr->data);
				curr= curr->next;
			}
			free(line);
			continue;
		}
		if(strstr(line, "!") == line){
			const char * query= line+1;
			char * match= NULL;
			if((&Log)->size > 0)
				match= log_search(&Log, query);
			if(match){
				printf("%s matches %s\n", query, match);
				char * line2= malloc(strlen(match)+1);
				strcpy(line2, match);
				log_push(&Log, line2);
				free(line);
				if(strstr(match, "cd ")){
					char * path= match + 3;
					int change= chdir(path);
					if(change == 0){
						int pid= getpid();
						printf("Command executed by pid=%d\n", pid);
						continue;
					}
					else{
						printf("%s: No such file or directory\n",path);
						continue;
					}
				}
				char * array[5]= {NULL, NULL, NULL, NULL, NULL};
				char * temp2= malloc(strlen(match)+1);
				strcpy(temp2, match);
				char * str= strtok(temp2, " ");
				if(!str){
					array[0] = malloc(strlen(temp2)+1);
					strcpy(array[0], temp2);
				}
				else{
					int i=0;
					for(i=0; i < 5 && str; i++){
						array[i]= malloc(strlen(str)+1);
						strcpy(array[i], str);
						str= strtok(NULL, " ");
					}
				}
				int status;
				pid_t child= fork();
				if(child >= 0){
					if(child == 0){
						execvp(array[0], array);
						printf("%s: not found\n", line);
						exit(1);
					}
					else{
						pid_t pid= getpid();
						while(pid != child){
							pid= wait(&status);
						}
						printf("Command executed by pid=%d\n", pid);
					}
				}
				else{
					printf("fork failed\n");
					exit(1);
				}
				free(temp2);
				free(str);
				int i=0;
				for(i=0; i < 5; i++){
					free(array[i]);
				}
				continue;
			}
			else printf("No Match\n");
			continue;
		}
		
		char * array[5]= {NULL, NULL, NULL, NULL, NULL};
		char * temp= malloc(strlen(line)+1);
		strcpy(temp, line);
		char * str= strtok(temp, " ");
		if(!str){
			array[0] = malloc(strlen(temp)+1);
			strcpy(array[0], temp);
		}
		else{
			int i=0;
			for(i=0; i < 5 && str; i++){
				array[i]= malloc(strlen(str)+1);
				strcpy(array[i], str);
				str= strtok(NULL, " ");
			}
		}
		int status;
		pid_t child= fork();
		if(child == 0){
			execvp(array[0], array);
			printf("%s: not found\n", temp);
			exit(1);
		}
		else{
			pid_t pid= getpid();
			while(pid != child){
				pid= wait(&status);
			}
			printf("Command executed by pid=%d\n", pid);
			log_push(&Log, line);
		}
		int i=0;
		for(i=0; i < 5; i++){
			free(array[i]);
		}
		free(str);
		free(temp);
	}
	return 0;
}
