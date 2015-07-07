/** @file alloc.c */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

size_t my_log2(size_t x){
	size_t count=0;
	while(x){
		x/=2;
		count++;
	}
	return --count;
}

typedef struct freeBlock{
	size_t size;
	struct freeBlock* next;
	struct freeBlock* prev;
}freeBlock;

typedef struct list{
	int count;
	freeBlock* head;
	freeBlock* tail;
}list;

//list* list_t;
int array_init= 0;

list* arrayList;

void init(){
	array_init= 1;
	arrayList = sbrk(33*sizeof(list));
	/*int i=0;
	for(i=0; i < 33; i++){
		arrayList[i].head= sbrk(sizeof(freeBlock));
		arrayList[i].tail= sbrk(sizeof(freeBlock));
		arrayList[i].head->size=sizeof(freeBlock);
		arrayList[i].tail->size=sizeof(freeBlock);
		arrayList[i].head->next= arrayList[i].tail;
		arrayList[i].head->prev= arrayList[i].tail;
		arrayList[i].tail->next= arrayList[i].head;
		arrayList[i].tail->prev= arrayList[i].head;
		arrayList[i].count=0;
	}*/
	return ;
}

void insert(freeBlock * curr){
	if(curr->size==0)
		printf("uh oh, size=0\n");
	size_t size= curr->size-sizeof(freeBlock);
	size_t logSize= my_log2(size);
	curr->next= arrayList[logSize].head->next;
	arrayList[logSize].head->next= curr;
	curr->prev= arrayList[logSize].head;
	curr->next->prev= curr;
	arrayList[logSize].count++;
}

void * findBlock(size_t size){
	size_t logSize= my_log2(size);
    //logSize++;
    /*printf("hello");
    if(logSize+1 > 32){
        printf("ahahaha");
        return NULL;
    }*/
    if(arrayList[logSize].head==NULL){
        arrayList[logSize].head= sbrk(sizeof(freeBlock));
        arrayList[logSize].tail= sbrk(sizeof(freeBlock));
        arrayList[logSize].head->next= arrayList[logSize].tail;
        arrayList[logSize].head->prev= arrayList[logSize].tail;
        arrayList[logSize].tail->next= arrayList[logSize].head;
        arrayList[logSize].tail->prev= arrayList[logSize].head;
        arrayList[logSize].count=0;
        return NULL;
    }
	if(arrayList[logSize].count!=0){
		freeBlock * curr= arrayList[logSize].head->next;
		if((curr->size - sizeof(freeBlock)) >= size){
			curr->next->prev=curr->prev;
			curr->prev->next= curr->next;
			arrayList[logSize].count --;
			return curr;
		}
	}
	return NULL;
}

/**
 * Allocate space for array in memory
 * 
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 * 
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size)
{
	/* Note: This function is complete. You do not need to modify it. */
	void *ptr = malloc(num * size);
	
	if (ptr)
		memset(ptr, 0x00, num * size);

	return ptr;
}


/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size)
{
	if(array_init==0)
		init();

	//printf("malloc size is %lu\n", size);
    freeBlock * block= findBlock(size);
    if(block){
        if((block->size - sizeof(freeBlock)) > (size +10240000)){
			//printf("found block and split\n");
            //freeBlock * ret= (freeBlock*)((char *)(block)-sizeof(freeBlock));
            freeBlock * new= (freeBlock*)((char *)(block) + size + sizeof(freeBlock));
            new->size= block->size - size - sizeof(freeBlock);
            block->size -= new->size;
            insert(new);
            return (void *)((char *)(block) + sizeof(freeBlock));
        }
        return (void *)((char*)(block) + sizeof(freeBlock));
	}	
	else if(!block){
        if(size < 200000 && arrayList[my_log2(size)].count < 5){
			freeBlock * ret= sbrk(2*(size+sizeof(freeBlock)));
			ret->size= 2*(size+sizeof(freeBlock));
            freeBlock * new= (freeBlock *)((char*)(ret) + size + sizeof(freeBlock));
            new->size= ret->size - size - sizeof(freeBlock);
            ret->size -= new->size;
            insert(new);
			return (void*)((char*)(ret)+sizeof(freeBlock));
        }
		freeBlock * ret= sbrk((size+sizeof(freeBlock)));
		ret->size= (size+sizeof(freeBlock));
		ret->next=NULL;
		ret->prev=NULL;
		return (void*)((char *)(ret) + sizeof(freeBlock));
	}
	printf("malloc\n");	
	return NULL;
}


/**
 * Deallocate space in memory
 * 
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr)
{
	//printf("freeing\n");
	// "If a null pointer is passed as argument, no action occurs."
	if (!ptr)
		return;
	freeBlock * temp=(freeBlock *)((char*)(ptr)-sizeof(freeBlock));
	insert(temp);
	
	return;
}


/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *    
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
//int count=0;
void *realloc(void *ptr, size_t size)
{//	count++;
//	printf("count= %d\n", count);
	 // "In case that ptr is NULL, the function behaves exactly as malloc()"
	if (!ptr)
		return malloc(size);

	 // "In case that the size is 0, the memory previously allocated in ptr
	 //  is deallocated as if a call to free() was made, and a NULL pointer
	 //  is returned."
	if (!size)
	{
		free(ptr);
		printf("realloc bad\n");
		return NULL;
	}

	freeBlock * temp= (freeBlock *) (((char *)(ptr)) - sizeof(freeBlock));
	
	if(size > (temp->size - sizeof(freeBlock))){
		freeBlock * new= malloc(size);
		memcpy((void*)new, ptr, (temp->size - sizeof(freeBlock)));
		free(ptr);
		return (void *)new;
	}
/*	freeBlock * newBlock;

//  exceeding 2gb
	if((temp->size - sizeof(freeBlock)) >= (size+64)){
		newBlock= (void *)((char*)(temp) + sizeof(freeBlock) + size);		
		newBlock->size= temp->size - size - sizeof(freeBlock);
		
		temp->size -= newBlock->size;
		free((void*)((char*)newBlock+sizeof(freeBlock)));
		return (void*)(temp)+sizeof(freeBlock);
	}*/
   if(temp->size - sizeof(freeBlock) >= size){
		return ptr;
	}

	return NULL;
}
