/***
	MALLOC and CALLOC and REALLOC
		These are error-checking allocation routines.
***/

#include <stdio.h>

void
*MALLOC(size)
	unsigned int size;
{
	void *ptr;
	
	if ((ptr = malloc(size)) == NULL) {
		fprintf(stderr,"Malloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


void
*CALLOC(num,size)
	unsigned int num,size;
{
	void *ptr;
	
	if ((ptr = calloc(num,size)) == NULL) {
		fprintf(stderr,"Calloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


void
*REALLOC(oldptr,size)
	void *oldptr;
	unsigned int size;
{
	void *ptr;
	
	if ((ptr = realloc(oldptr,size)) == NULL) {
		fprintf(stderr,"Realloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


FREE(ptr)

     char *ptr;
{
  free(ptr);
}


CFREE(ptr,n,size)
     char *ptr;
     unsigned int n,size;
{
  free(ptr);
}


