#include <stdio.h>
#include <stdlib.h>

#ifdef PART2
  #define NUM_CYCLES 10000000
#else
  #define NUM_CYCLES 1
#endif
//#include "alloc2.c"
int main()
{
  int i;
  for (i = 0; i < NUM_CYCLES; i++) {
    int *ptr = malloc(sizeof(int));
//    printf("hello\n");
    if (ptr == NULL)
    {
      printf("Memory failed to allocate!\n");
      return 1;
    }

    *ptr = 4;
    if(*ptr!=4)
        printf("wrong data\n");
    free(ptr);
  }

	printf("Memory was allocated, used, and freed!\n");	
	return 0;
}
