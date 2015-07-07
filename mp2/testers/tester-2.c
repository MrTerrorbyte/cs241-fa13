#include <stdio.h>
#include <stdlib.h>

#define M (1024 * 1024)
#define K (1024)

#ifdef PART2
  #define TOTAL_ALLOCS 5*M
#else
  #define TOTAL_ALLOCS 50*K
#endif
//#include "../alloc.c"
int main()
{
    malloc(1);
    //printf("small malloc\n");
	int i;
	int **arr = malloc(TOTAL_ALLOCS * sizeof(int *));

    if (arr == NULL)
	{
	    printf("malloc(big)\n");
		printf("Memory failed to allocate!\n");
		return 1;
	}

	for (i = 0; i < TOTAL_ALLOCS; i++)
	{
        //printf("hello\n");
		arr[i] = malloc(sizeof(int));
        //printf("many mallocs");
		if (arr[i] == NULL)
		{
			printf("Memory failed to allocate!\n");
			return 1;
		}
		
		*(arr[i]) = i;
	}

	for (i = 0; i < TOTAL_ALLOCS; i++)
	{
        if (*(arr[i]) != i)
		{
		    //printf("Total_Allocs=%d\n", TOTAL_ALLOCS);
            printf("arr[i]= %d\n", *(arr[i]));
            printf("Memory failed to contain correct data after many allocations!\n");
			return 2;
		}
        //else printf("arr[i]= %d\n", *(arr[i]));
	}

	for (i = 0; i < TOTAL_ALLOCS; i++)
		free(arr[i]);

	free(arr);
	printf("Memory was allocated, used, and freed!\n");	
	return 0;
}
