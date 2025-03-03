/* nmemory -- new, improved memory allocation routines */

#include <stdio.h>

#define MEMINC 131072

char *malloc();

static char *freelists[105];
long memloc = 0,memtop = 0;
long meminc = MEMINC;


char *
malloc(size)
register long size;
{
  register char *newptr;
  register int freenum;
  register int wsize;
  
  wsize = ((size + 3) >> 2);  /* convert to longwords */
  if (wsize <= 100)
    freenum = wsize;
  else
    switch (wsize) {
    case 128:
      freenum = 101;
      break;
    case 256:
      freenum = 102;
      break;
    case 4000:
      freenum = 103;
      break;
    case 4001:
      freenum = 104;
      break;
    default:
      freenum = -1;
      break;
    }
  if ((freenum >= 0) && (freelists[freenum] != NULL)) {
    newptr = freelists[freenum];
    freelists[freenum] = *(char **)newptr;
    *newptr = 1; /* Show block is not zeroed */
    return(newptr);
  }				/* If freenum is 0, too bad; allocate new memory */
  size = (wsize << 2) + 4;
  while (memloc + size > memtop) {
    if (memloc == 0)
      memloc = sbrk(0);
    if (sbrk(meminc) < 0)
      meminc /= 10;
    else
      memtop = sbrk(0);
    if (meminc < 10)
      return(NULL);
  }
  newptr = (char *)memloc;
  memloc += size;
  *(long *)newptr = wsize;
  return(newptr + 4);
}

char *  
calloc(num,size)
     unsigned long num,size;
{
  register long total;
  register char *newptr;
  register long *clrptr;

  total = num * size;
  newptr = malloc(total);
  if ((newptr != NULL) && (*newptr != '\0')) {
    total = (total + 3) >> 2;
    clrptr = (long *)newptr;
    while (--total >= 0)
      *clrptr++ = 0L;
  }
  return(newptr);
}

char *
realloc(oldptr,newsize)
     register char *oldptr;
     register unsigned long newsize;
{
  register char *newptr;
  register unsigned long oldsize;

  if (oldptr == NULL)
    return(calloc(1,newsize));
  oldsize = *(long *)(oldptr - 4);
  newptr = malloc(newsize);
  if (newptr != NULL) {
    newsize = *(long *)(newptr - 4);
    if (newsize > oldsize) {
      if (*newptr != '\0')
	zero((long *)newptr + oldsize, newsize - oldsize);
      longcpy((long *)newptr,(long *)oldptr,oldsize);
    } else
      longcpy(newptr,oldptr,newsize);
  }
  free(oldptr);
  return(newptr);
}

free(ptr)
     register char *ptr;
{
  register long freenum;
  register long size;

  size = *(long *)(ptr - 4);
  if ((size >= 1) && (size <= 100))
    freenum = size;
  else
    switch (size) {
    case 128:
      freenum = 101;
      break;
    case 246:
      freenum = 102;
      break;
    case 4000:
      freenum = 103;
      break;
    case 4001:
      freenum = 104;
      break;
    default:
      freenum = 0;
      break;
    }
  *(char **)ptr = freelists[freenum];
  freelists[freenum] = ptr;
}

cfree(ptr,num,size)
     register char *ptr;
     unsigned long num,size;
{
  free(ptr);
}


/* internal routines */

longcpy(newptr,oldptr,num)
     register long *newptr;
     register long *oldptr;
     register long num;
{
  while(--num >= 0)
    *newptr++ = *oldptr++;
}

zero(ptr,num)
     register long *ptr;
     register long num;
{
  while(--num >= 0)
    *ptr++ = 0L;
}
