/** @file log.h */

#ifndef __LOG_H_
#define __LOG_H_

typedef struct log_entry{
	char * data;
	struct log_entry * next;
	struct log_entry * prev;
} log_entry;

typedef struct _log_t {
	log_entry * head;
	log_entry * tail;
	int size;
} log_t;

void log_init(log_t *l);
void log_destroy(log_t* l);
void log_push(log_t* l, const char *item);
char * log_search(log_t* l, const char *prefix);
/*char * log_pop(log_t * l);
char * log_at(log_t * l, unsigned int idx);
*/
#endif
