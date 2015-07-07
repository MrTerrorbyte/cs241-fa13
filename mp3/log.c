/** @file log.c */
#include <stdlib.h>
#include <string.h>
#include "log.h"

/**
 * Initializes the log.
 *
 * You may assuem that:
 * - This function will only be called once per instance of log_t.
 * - This function will be the first function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @returns
 *   The initialized log structure.
 */
void log_init(log_t *l) {
	l->head= NULL;
	l->tail= NULL;
	l->size= 0;
}

/**
 * Frees all internal memory associated with the log.
 *
 * You may assume that:
 * - This function will be called once per instance of log_t.
 * - This funciton will be the last function called per instance of log_t.
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure to be destoryed.
 */
void log_destroy(log_t* l) {
	log_entry * temp;
	log_entry * curr;
	curr= l->head;
	while(curr!=NULL){
		if(curr->data){
			free(curr->data);
			curr->data=NULL;
		}
		temp= curr->next;
		free(curr);
		curr= temp;
	}
}

/**
 * Push an item to the log stack.
 *
 *
 * You may assume that:
* - All pointers will be valid, non-NULL pointer.
*
 * @param l
 *    Pointer to the log data structure.
 * @param item
 *    Pointer to a string to be added to the log.
 */
void log_push(log_t* l, const char *item) {
	//printf("item= %s \n", item);	
	log_entry * new= calloc(sizeof(log_entry),1);
	
	if(l->size == 0){
		new->data= item;
		l->head= new;
		l->tail= new;
	}

	else{
		l->tail->next= new;
		new->prev= l->tail;
		new->next=NULL;
		new->data= item;
		l->tail= new;
	}
	l->size++;
}


/**
 * Preforms a newest-to-oldest search of log entries for an entry matching a
 * given prefix.
 *
 * This search starts with the most recent entry in the log and
 * compares each entry to determine if the query is a prefix of the log entry.
 * Upon reaching a match, a pointer to that element is returned.  If no match
 * is found, a NULL pointer is returned.
 *
 *
 * You may assume that:
 * - All pointers will be valid, non-NULL pointer.
 *
 * @param l
 *    Pointer to the log data structure.
 * @param prefix
 *    The prefix to test each entry in the log for a match.
 *
 * @returns
 *    The newest entry in the log whose string matches the specified prefix.
 *    If no strings has the specified prefix, NULL is returned.
 */
char *log_search(log_t* l, const char *prefix) {
	log_entry * curr= l->tail;
	while(curr){
		char * str= strstr(curr->data, prefix);
		if(str==curr->data)
			return curr->data;
		else curr= curr->prev;
	}
    return NULL;
}
