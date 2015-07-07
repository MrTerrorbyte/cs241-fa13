/** @file libpriqueue.h
 */

#ifndef LIBPRIQUEUE_H_
#define LIBPRIQUEUE_H_

/**
  Priqueue Data Structure
*/
typedef struct node_t{
	struct node_t * next;
	void * data;
}node_t;

typedef struct _priqueue_t
{
	int size;
	node_t * head;
	int (*comp)(const void * ptr1, const void * ptr2);
} priqueue_t;


void   priqueue_init     (priqueue_t *q, int(*comparer)(const void *, const void *));

int    priqueue_offer    (priqueue_t *q, void *ptr);
void * priqueue_peek     (priqueue_t *q);
void * priqueue_poll     (priqueue_t *q);
int    priqueue_size     (priqueue_t *q);
void   priqueue_remove	 (priqueue_t * q, void * ptr); //not graded
void   priqueue_destroy  (priqueue_t *q);

#endif /* LIBPQUEUE_H_ */
