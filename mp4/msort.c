/** @file msort.c */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct t_arg{
	int * start;
	int numInThisThread;
	int numInNextThread;
	int numTotalElems;
	int dup;
}t_args;

int compare(const void * ptr1, const void * ptr2){
	if(*(int *)ptr1 < *(int *)ptr2)
		return -1;
	else if(*(int *)ptr1 == *(int *)ptr2)
		return 0;
	return 1;
}	
/*
int count(int * start){
		int ret=0;
		while(*start!=0){
			ret++;
			start++;
		}
		return ret;
}*/

void * merge(t_args * args){
	args->dup=0;
	int * ptr1= args->start;
	int * ptr2= args->start + args->numInThisThread;
	//printf("in merge, ptr1= %d\n", *ptr1);
	//printf("in merge, ptr2= %d\n", *ptr2);
	//printf("in merge, args->numInNextThread= %d\n", args->numInNextThread);

	int * outOfBound1= ptr2;
	int * outOfBound2= ptr2 + args->numInNextThread;
	int * temp= malloc(args->numTotalElems * sizeof(int));
/*	if(*ptr2 == 0){
		ptr2= outOfBound2;
		printf("ptr2 is outOfBounds from beginning\n");
	}
*/
	int i=0;
	for( ; ptr1 != outOfBound1 || ptr2 != outOfBound2; i++){
		if(ptr1 == outOfBound1){
			temp[i]= *ptr2;
			ptr2++;
		}
		else if(ptr2 == outOfBound2){
			temp[i]= *ptr1;
			ptr1++;
		}
		else if(*ptr1 == *ptr2){
			args->dup++;
			temp[i]= *ptr1;
			ptr1++;
		}
		else if(*ptr1 > *ptr2){
			temp[i]= *ptr2;
			ptr2++;
		}
		else if(*ptr1 < *ptr2){
			temp[i]= *ptr1;
			ptr1++;
		}
	}

	int * tempA= args->start;
	for(i=0; i < args->numInThisThread + args->numInNextThread; i++){
		*tempA= temp[i];
		tempA++;;
	}

	free(temp);
	//printf("haha\n");
	fprintf(stderr, "Merged %d and %d elements with %d duplicates.\n", args->numInThisThread, args->numInNextThread, args->dup);
	
	return NULL;
}

void * sort(t_args * args){
	//printf("args->start= %d\n", *((args)->start));
	qsort((args)->start, (args)->numInThisThread, sizeof(int), compare);
	fprintf(stderr, "Sorted %d elements.\n", (args)->numInThisThread);

    //printf("address is %p\n", args);
	return NULL;
}

int main(int argc, char **argv){
/*	clock_t start, end;
	double cpu_time_used;
	start= clock();
*/
	int * inputArray= malloc(sizeof(int)*100000000);
	if(argc <= 0){
		free(inputArray);
		return 0;
	}
	if(argc > 0){
		char * line= NULL;
		size_t size= 0;
		int i=0;
		for( ; !feof(stdin); i++){
			getline(&line, &size, stdin);
			int temp= atoi(line);
			inputArray[i]= temp;
			//printf("inputArray[%d]= %d\n", i, inputArray[i]);
		}
		free(line);
		inputArray[i-1]= 0; //getline is reading the last line twice
		int segment_count= atoi(argv[1]);
		int numElemsPerThread= 0;
		int total= --i; //getline read the last line twice, i is one too big
		if(total % segment_count == 0)
			numElemsPerThread= total/segment_count;
		else
			numElemsPerThread= (total/segment_count)+1;
		if(segment_count == 1){
			qsort(&inputArray[0], total, sizeof(int), compare);
			printf("Sorted %d elements.\n", total);
			int k=0;
			for( ; k < total; k++)
				printf("%d\n", inputArray[k]);
			return 0;
		}
		pthread_t * threadsArray= malloc(sizeof(pthread_t) * segment_count);
		t_args * argsArray= malloc(sizeof(t_args) * segment_count);

		int k=0;


		if(total % segment_count != 0){
			for( ; k < (segment_count - 1); k++){
				argsArray[k].start= &inputArray[k*numElemsPerThread];
				argsArray[k].numInThisThread= numElemsPerThread;
				if((k+1) < (segment_count - 1))
					argsArray[k].numInNextThread= numElemsPerThread;
				else {
					argsArray[k].numInNextThread= total - (segment_count -1) * (total/segment_count + 1);
				}
				argsArray[k].numTotalElems= total;
			}
			argsArray[k].start= &inputArray[k*numElemsPerThread];
			argsArray[k].numInThisThread= argsArray[k-1].numInNextThread;
			for(k=0; k < segment_count; k++){
				pthread_create(&threadsArray[k], NULL, (void *)*sort, &argsArray[k]);
			}
			for(k=0; k < segment_count; k++){
				pthread_join(threadsArray[k], NULL);
			}
			while(segment_count > 1){
				int temp= segment_count;
				int j=0;
					for(k=0; k < temp/2; k++){
						pthread_create(&threadsArray[k], NULL, (void *)*merge, &argsArray[j]);
						j+=2;
						segment_count--;
					}
					for(k=0; k < temp/2; k++){
						pthread_join(threadsArray[k], NULL);
					}
					if(segment_count == 1)
						break;
					argsArray[0].numInThisThread += argsArray[0].numInNextThread;
				
					for(k=1; k < segment_count-1; k++){
						argsArray[k].start= argsArray[k-1].start + argsArray[k-1].numInThisThread;
						argsArray[k].numInThisThread += argsArray[k].numInNextThread;
						argsArray[k-1].numInNextThread= argsArray[k].numInThisThread;
					}
					argsArray[k].start= argsArray[k-1].start + argsArray[k-1].numInThisThread;


					//argsArray[k].numInThisThread= count(argsArray[k].start);
					int * curr= argsArray[k].start;
					int count= 0;
					for( ; curr!=argsArray[0].start; curr--){
						count++;
					}
					argsArray[k].numInThisThread= total-count;
					argsArray[k].numInNextThread= 0;
					argsArray[k-1].numInNextThread= argsArray[k].numInThisThread;	

			}
						
		}
		
		
		
		else if(total % segment_count == 0){
			for( ; k < segment_count; k++){
				argsArray[k].start= &inputArray[k*numElemsPerThread];
				argsArray[k].numInThisThread= numElemsPerThread;
				if((k+1) < segment_count)
					argsArray[k].numInNextThread= numElemsPerThread;
				else argsArray[k].numInNextThread= 0;
				argsArray[k].numTotalElems= total;
			}
			for(k=0; k < segment_count; k++){
				pthread_create(&threadsArray[k], NULL, (void *)*sort, &argsArray[k]);
			}
			for(k=0; k < segment_count; k++){
				pthread_join(threadsArray[k], NULL);
			}
		
			while(segment_count > 1){
				
				int temp= segment_count;
				int j=0;
					for(k=0; k < temp/2; k++){
						pthread_create(&threadsArray[k], NULL, (void *)*merge, &argsArray[j]);
						j+=2;
						segment_count--;
					}
			
					for(k=0; k < temp/2; k++){
						pthread_join(threadsArray[k], NULL);
					}

					if(segment_count == 1)
						break;
			
					argsArray[0].numInThisThread += argsArray[0].numInNextThread;
				
					for(k=1; k < segment_count-1; k++){
						argsArray[k].start= argsArray[k-1].start + argsArray[k-1].numInThisThread;
						argsArray[k].numInThisThread *=2;
						argsArray[k-1].numInNextThread= argsArray[k].numInThisThread;
					}
					argsArray[k].start= argsArray[k-1].start + argsArray[k-1].numInThisThread;


					//argsArray[k].numInThisThread= count(argsArray[k].start);
					int * curr= argsArray[k].start;
					int count= 0;
					for( ; curr!=argsArray[0].start; curr--){
						count++;
					}
					argsArray[k].numInThisThread= total-count;
					argsArray[k].numInNextThread= 0;
					argsArray[k-1].numInNextThread= argsArray[k].numInThisThread;	
			}
		}

		for(k=0; k < total; k++)
			printf("%d\n", inputArray[k]);
	
	free(threadsArray);
	free(argsArray);
	free(inputArray);
	}
	

/*
	end= clock();
	cpu_time_used = ((double) (end - start))/CLOCKS_PER_SEC;
	printf("time took: %f\n", cpu_time_used);*/
	return 0;
}


