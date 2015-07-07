/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"


/**
  Initializes the priqueue_t data structure.
  
  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	q->head= NULL;
	q->size= 0;
	q->comp= comparer;
}


/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	node_t * newNode= malloc(sizeof(node_t));
	newNode->data= ptr;
	newNode->next= NULL;
	
	if(!(q->head)){
		q->head= newNode;
		q->size++;
		return 0;
	}
	if(q->comp(newNode->data, q->head->data) < 0){
		newNode->next= q->head;
		q->head= newNode;
		q->size++;
		return 0;
	}

	node_t * curr= q->head;
	int i=1;
	for( ; i <= q->size; i++){
		if(!(curr->next) || q->comp(newNode->data, curr->next->data) < 0 ){
			newNode->next= curr->next;
			curr->next= newNode;
			q->size++;
			return i;
		}
		else curr= curr->next;
	}
	
	return -1;
}


/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if(!(q->size))
		return NULL;
	return q->head->data;
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */

void *priqueue_poll(priqueue_t *q)
{
	if(q->size == 0)
		return NULL;
	node_t * ret= q->head;
	q->head= q->head->next;
	ret->next= NULL;
	q->size--;
	void * temp= ret->data;
	free(ret);
	return temp;
}

void priqueue_remove(priqueue_t * q, void * ptr){

	while(q->size > 0){
		if(q->head == ptr){
			priqueue_poll(q);
		}
		else break;
	}
	node_t * curr= q->head;
	node_t * temp;
	if(q->size > 0)
	while(curr->next){
		if(curr->next->data == ptr){
			temp= curr->next;
			curr->next= curr->next->next;
			q->size--;
			free(temp);
		}
		curr= curr->next;
	}
}


/**
  Returns the number of elements in the queue.
 
  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.
  
  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	node_t * curr= q->head;
	node_t * temp= curr;

	while(q->size){
		temp= curr->next;
		free(curr);
		curr= temp;
		q->size--;
	}
	//free(q);
}
