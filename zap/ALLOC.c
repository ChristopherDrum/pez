/***
	MALLOC and CALLOC and REALLOC
		These are error-checking allocation routines.
***/

#include <stdio.h>
#include <stdlib.h>

char *MALLOC(unsigned int size)
{
	char *ptr;
	
	if ((ptr = malloc(size)) == NULL) {
		fprintf(stderr,"Malloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


char *CALLOC(unsigned int num, unsigned int size)
{
	char *ptr;
	
	if ((ptr = calloc(num,size)) == NULL) {
		fprintf(stderr,"Calloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


char *REALLOC(void *oldptr, unsigned int size)
{
	char *ptr;
	
	if ((ptr = realloc(oldptr,size)) == NULL) {
		fprintf(stderr,"Realloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


void FREE(void *ptr)
{
  free(ptr);
}


int CFREE(void *ptr, unsigned int n, unsigned int size)
{
  free(ptr);//this is all cfree was doing!
}


