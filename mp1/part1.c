/** @file part1.c */

/*
 * Machine Problem #1
 * CS 241 Fall2013
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mp1-functions.h"

/**
 * (Edit this function to print out the ten "Illinois" lines in mp1-functions.c in order.)
 */
int main()
{

	first_step(81);
	
	int * two= malloc(sizeof(int));
	*two= 132;
	second_step(two);
	free(two);
	
	int ** three= malloc(sizeof(int *));
	three[0]= malloc(sizeof(int));
	*three[0]= 8942;
	double_step(three);
	free(three[0]);
	free(three);

	int * four=0;	
	strange_step(four);

	char * five= calloc(4,sizeof(char));
	empty_step((void*)(five));
	free(five);

	char * s2= calloc(4,sizeof(char));
	s2[3]='u';
	void * s= s2;
	two_step(s, s2);
	free(s2);

	char * first= 0;
	char * second= first+2;
	char * third= second+2;
	three_step(first, second, third);	

	first= calloc(2,sizeof(char));
	first[1]= 0;
	second= malloc(3*sizeof(char));
	second[2]= first[1]+8;
	third= malloc(4*sizeof(char));
	third[3]= second[2]+8;
	step_step_step(first, second, third);
	free(first);
	free(second);
	free(third);

	char * nine= malloc(sizeof(char));
	*nine= 1;
	it_may_be_odd(nine, 1);	
	free(nine);

	int * orange= malloc(sizeof(int));
	*orange= 513;
	int * blue= orange;
	the_end((void *)orange,(void *) blue);	
	//printf("temp= %d\n", (*temp));
	free(orange);
	return 0;
}
