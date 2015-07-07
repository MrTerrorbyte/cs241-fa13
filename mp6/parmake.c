/** @file parmake.c */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "rule.h"
#include "queue.h"
#include "parser.h"

queue_t allRules;
queue_t rulesQueue;
pthread_mutex_t mutex;
pthread_cond_t cond;

typedef struct{
	rule_t * rule;
	int ready;
}rule_struct;

void new_target(char * target){
	rule_struct * myRule= calloc(1,sizeof(rule_struct));
	myRule->rule= malloc(sizeof(rule_t));
	myRule->ready= 0;
	rule_init(myRule->rule);
	myRule->rule->target= strdup(target);
	queue_enqueue(&rulesQueue, myRule);
	queue_enqueue(&allRules, myRule);
}

void new_dependency(char * target, char * dependency){
	int i=0;
	char * dep= strdup(dependency);
	for( ; i < queue_size(&rulesQueue); i++){
		rule_struct * tempRule= queue_at(&rulesQueue, i);
		char * tempTarget= tempRule->rule->target;
		if(strcmp(tempTarget, target) == 0){
			queue_enqueue(tempRule->rule->deps, dep);
			break;
		}
	}
}

void new_command(char * target, char * command){
	int i=0;
	char * com= strdup(command);
	for( ; i < queue_size(&rulesQueue); i++){
		rule_struct * tempRule= queue_at(&rulesQueue, i);
		char * tempTarget= tempRule->rule->target;
		if(strcmp(tempTarget, target) == 0){
			queue_enqueue(tempRule->rule->commands, com);
			break;
		}
	}
}

int isRule(char * ptr){
	int i= 0;
	rule_struct * myRule= NULL;
	for( ; i < queue_size(&allRules); i++){
		myRule= queue_at(&allRules, i);
		if(strcmp(ptr, myRule->rule->target) == 0){
			return 1;
		}
	}
	return 0;
}

int ruleReady(char * rule2){
	int i= 0;
	rule_struct * myRule;
	int ret= 0;
	for( ; i < queue_size(&allRules); i++){
		myRule= queue_at(&allRules, i);
		if(myRule->ready == 1 && strcmp(myRule->rule->target, rule2) == 0){
			ret= 1;
			break;
		}
	}
	return ret;
}

int depsReady(rule_struct * myRule){
	int i= 0;
	char * currDep =NULL;
	for( ; i < queue_size(myRule->rule->deps); i++){
		currDep= queue_at(myRule->rule->deps, i);
		if(isRule(currDep)){
			if(ruleReady(currDep) == 0)
				return 0;
		}
	}
	return 1;
}

int depsAreFiles(queue_t * dep){
	int i= 0;
	if(queue_size(dep) <= 0)
		return 0;
	for( ; i < queue_size(dep); i++){
		if(isRule(queue_at(dep, i)))
			return 0;
	}
	return 1;
}

rule_struct * getRule(){
	int i= 0;
	rule_struct * myRule= NULL;
	for( ; i < queue_size(&rulesQueue); i++){
		myRule= queue_at(&rulesQueue, i);
		if(queue_size(myRule->rule->deps) == 0 || depsReady(myRule)){
			return queue_remove_at(&rulesQueue, i);
		}
	}
	return NULL;
}

void runRule(rule_struct * myRule){
	int i= 0;
	char * com;
	for( ; i < queue_size(myRule->rule->commands); i++){
		com= queue_at(myRule->rule->commands, i);
		//printf("%s\n", com);
		if(system(com) != 0){
			exit(1);
		}
	}
}

void * start_routine(void * ptr){
	rule_struct * myRule= NULL;
	while(queue_size(&rulesQueue) > 0){
		pthread_mutex_lock(&mutex);
		while((myRule = getRule()) == NULL){
			if(queue_size(&rulesQueue))
				pthread_cond_wait(&cond, &mutex);
			else break;
		}
		pthread_mutex_unlock(&mutex);
		//
		if(myRule == NULL)
			return NULL;
		//
		if(depsAreFiles(myRule->rule->deps)){
			if(access(myRule->rule->target, F_OK) == -1){
				runRule(myRule);
			}
			else{
				struct stat stat1, stat2;
				stat(myRule->rule->target, &stat1);
				int i= 0;
				for( ; i < queue_size(myRule->rule->deps); i++){
					stat(queue_at(myRule->rule->deps, i), &stat2);
					if(stat2.st_mtime > stat1.st_mtime){
						runRule(myRule);
						break;
					}
				}
			}
		}
		else {
			runRule(myRule);
		}
		//printf("queue_size= %d\n", queue_size(&rulesQueue));
		pthread_mutex_lock(&mutex);
		myRule->ready= 1;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

/**
 * Entry point to parmake.
 */
int main(int argc, char **argv)
{
	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex, NULL);
	queue_init(&allRules);	
	queue_init(&rulesQueue);
	int arg;
	int numThreads= 1; //1 by default
	char * makeFile= NULL;
	char * make;
	char ** targets= malloc(sizeof(char *));
	targets[0]= '\0';
	while((arg=getopt(argc, argv, "j:f:"))!= -1){
		if(arg == 'j'){
			numThreads= atoi(optarg);
		}
		else if(arg == 'f')
			makeFile= optarg;
	}
	int size= argc - optind;
	if(optind < argc){
		targets= realloc(targets, sizeof(char *) * (size + 1));
		int i= 0;
		for( ; i < size; i++){
			targets[i]= malloc(sizeof(char) * strlen(argv[optind]+1));
			strcpy(targets[i], argv[optind]);
			optind++;
		}
		targets[i]= '\0';
	}
	if(!makeFile){
		if(access("./makefile", F_OK) == 0){
			make= "makefile\0";
			makeFile= make;
		}
		else if(access("./Makefile", F_OK) == 0){
			make= "Makefile\0";
			makeFile= make;
		}
		else return -1;
	}
	parser_parse_makefile(makeFile, targets, &new_target, &new_dependency, &new_command);
	/*
	int b= 0;
	for( ; b < queue_size(&rulesQueue); b++){
		rule_struct * temp= queue_at(&rulesQueue, b);
		printf("%s     ", temp->rule->target);
	}*/
	pthread_t * threadsArray= malloc(sizeof(pthread_t) * numThreads);
	int i= 0;
	for( ; i < numThreads; i++)
		pthread_create(&threadsArray[i], NULL, start_routine, NULL);
	for(i= 0; i < numThreads; i++)
		pthread_join(threadsArray[i], NULL);

	
	pthread_mutex_destroy(&mutex);
	
	//Everything after this is for freeing
	
	free(threadsArray);
	rule_struct * currRule;
	char * currDep;
	char * currTar;

	int j, k;
	for(i= 0; i < queue_size(&allRules); i++){
		currRule= queue_at(&allRules, i);
		for(j=0; j < queue_size(currRule->rule->deps); j++){
			currDep= queue_at(currRule->rule->deps, j);
			free(currDep);
			//printf("freeing dep\n");
		}
		for(k=0; k < queue_size(currRule->rule->commands); k++){
			currTar= queue_at(currRule->rule->commands, k);
			free(currTar);
			//printf("freeing tar\n");
		}
		queue_destroy(currRule->rule->deps);
		queue_destroy(currRule->rule->commands);
		free(currRule->rule->deps);
		free(currRule->rule->commands);
		free(currRule->rule->target);
		free(currRule->rule);
		free(currRule);
	}
	
	queue_destroy(&allRules);
	if(size != 0){
		for(i=0; i < (size+1); i++)
			free(targets[i]);
	}
	free(targets);
	return 0; 
}

