/***
	MALLOC and CALLOC and REALLOC
		These are error-checking allocation routines.
***/

#include <stdio.h>

extern char *malloc();
extern char *calloc();
extern char *realloc();

char
*MALLOC(size)
	unsigned int size;
{
	char *ptr;
	
	if ((ptr = malloc(size)) == NULL) {
		fprintf(stderr,"Malloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


char
*CALLOC(num,size)
	unsigned int num,size;
{
	char *ptr;
	
	if ((ptr = calloc(num,size)) == NULL) {
		fprintf(stderr,"Calloc failed!!\n");
		exit(1);
	}
	return(ptr);
}


char
*REALLOC(oldptr,size)
	char *oldptr;
	unsigned int size;
{
	char *ptr;
	
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
  cfree(ptr,n,size);
}


