/*
  Segment file generation 
  By Matt Hillman
*/

#include <stdio.h>
#include <fcntl.h>
#include "zap.h"

extern char *CALLOC();
extern char *MALLOC();
extern LGSYMBOL *symlookup();
extern SYMBOL *symenter();
extern long funct_start;
extern STABLE *Gblsymtab;
extern UWORD  *top_page;
extern STABLE *Segsymtab;

char *itoa();
char *setadj();

static SEGMENT **segments = NULL;   /* List of segments */
static UWORD numsegs = 1;           /* Number of segments; 1 reserved for seg 0 */
static UWORD numspaces = 0;         /* Number of spaces available in seg. list */
static SEGMENT **opensegs = NULL;   /* List of open segments */
static UWORD openlsize = 0;         /* Size of open seg. list */
static UWORD numopen = 0;           /* Number of open segments */
static ITEM *funlist;               /* List of all functions */
static SEGNODE *adjacents;          /* List of adjacent segment pairs */
static char *adjtable;              /* Adjacency table */
static int segfd;                   /* Segment file descriptor */
static char *segbufs[SEGBNUM];      /* Segment file output buffers */
static long segbuf = 0;             /* Current output buffer */
static long segloc = 0;             /* Current location in buffer */
/* The bit array is used in manipulating bit tables.  See zapdoc.txt */
static char bit[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
static UWORD endlod_page;           /* Last page of impure and parser tables */
static UWORD funct_page;            /* First page of functions */

/* This remembers the beginning and end of segments, for cases where pure
   tables are in other than segment 0. */
void add_marker(seg, pc, first)
SEGMENT *seg;
long pc;
int first;
{
  segment_marker *chain = seg->segs, *nchain;
  if (first) {
    /* If this is already open, don't close it */
    if (chain && (chain->ending_pc == 0)) return;
    nchain = (segment_marker *)MALLOC(sizeof(segment_marker));
    nchain->next = chain;
    nchain->beginning_pc = pc;
    nchain->ending_pc = 0;
    seg->segs = nchain; }
  else if (!chain || (chain->ending_pc != 0)) {
    my_fprintf(stderr, "Closing non-open segment at %d.\n", pc);
    my_exit(1); }
  else
    chain->ending_pc = pc; }

/* This function puts a segment on the open list. */

segopen(seg)
AREG1 char *seg;
{
  DREG1 int segnum;

  if (++numopen > openlsize) { /* Expand list if necessary */
    openlsize += NSEGS;
    opensegs = (SEGMENT **)REALLOC(opensegs,(openlsize * sizeof(SEGMENT *))); }
  segnum = getnum(seg);
  opensegs[numopen - 1] = segments[segnum];  /* Add to open list */
  add_marker(segments[segnum], objtell(), 1);
}


/* This function closes the segments on the open list.  Just set the number of open
segments to 0 */

segclose()
{
  int i;
  for (i = 0; i < numopen; i++)
    add_marker(opensegs[i], objtell(), 0);
  numopen = 0;
}


/* This function puts a pair of functions onto the adjacency list. Later this list will
be converted into a table. Getnum(seg1) must be called twice because if getnum learns
about a new segment at getnum(seg2), seg1's number might change (if seg2 is the startup
segment). */
addadj(seg1,seg2)
     AREG1 char *seg1;
     AREG2 char *seg2;
{
  DREG1 int segnum1;
  DREG1 int segnum2;

  getnum(seg1); /* Make sure it knows about seg1 */
  segnum2 = getnum(seg2);
  segnum1 = getnum(seg1); /* Now really get segment 1's number. */
  addnode(&adjacents,segments[segnum1],segments[segnum2]);
}


/* This function adds a function to the function list of each segment
   on the open list. It also adds the function to the list of all functions. */

addfun(sym)
     AREG1 LGSYMBOL *sym;
{
  DREG1 int i;

  additem(&funlist,sym); /* Add function to list of all functions. */
  for (i = 0; i < numopen; additem(&opensegs[i++]->functions,sym));
}


/* This function adds a string to the string list of each segment
   on the open list. */

addstr(sym)
     AREG1 LGSYMBOL *sym;
{
  DREG1 int i;

  for (i = 0; i < numopen; additem(&opensegs[i++]->strings,sym));
}


/* This function adds a function or string to a functions list of used functions
   and strings */

addcall(fun,call)
     AREG1 LGSYMBOL *fun;
     AREG2 LGSYMBOL *call;
{
  if (call->lg_space == FUNCTION_SPACE)
    if (call->lg_add_info.function.temp.lastcall != fun) {
      call->lg_add_info.function.temp.lastcall = fun;
      additem(&fun->lg_add_info.function.calls,call);
    } else
      if (call->lg_space == ZSTRING_SPACE)
	if (call->lg_add_info.string.last_use != fun) {
	  call->lg_add_info.string.last_use = fun;
	  additem(&fun->lg_add_info.function.calls,call);
	}
}


/* This function returns a segment's number, allocating space if it's the first
time it's seen this segment. It knows it's seen the segment if a number greater than
0 is in the name string's 4-byte header. */

int
getnum(seg)
     AREG1 char *seg;
{
  AREG2 int *segnumP;

  segnumP = (int *)(seg-4);  /* Literal string header */
  if (*segnumP == 0) {
    if (++numsegs > numspaces) { /* Allocate more space */
      numspaces += NSEGS;
      segments = (SEGMENT **)REALLOC(segments,(numspaces * sizeof(SEGMENT *)));
    }
    if (strcmp(seg,"0") == 0) { /* First position reserved for segment 0 */
      *segnumP = 1;
      segments[0] = (SEGMENT *)CALLOC(1,sizeof(SEGMENT));
      segments[0]->name = seg;
      numsegs--;  /* Don't increase this for segment 0 */
    } else {
      *segnumP = numsegs;  /* Put segment number +1 in header. */
      segments[*segnumP - 1] = (SEGMENT *)CALLOC(1,sizeof(SEGMENT));
      segments[*segnumP - 1]->name = seg;
    }
  }
  return(*segnumP - 1);
}

/* This procedure makes the given segment the startup segment(# 1) It does this by
exchanging the startup segment's position with the segment in position 1, unless the
startup segment already is in position 1. */

segstartup(seg)
     AREG1 char *seg;
{
  AREG2 SEGMENT *temp;
  int num;

  num = getnum(seg);
  if (strcmp(seg,"0") == 0) {
    my_fprintf(stderr,"Startup segment cannot be segment '0'.\n");
    my_exit(1);
  }
  if (num != 1) {
    temp = segments[1];
    segments[1] = segments[num];
    segments[num] = temp;
    *(int *)(segments[1]->name - 4) = 2;  /* Segment # 1 */
    *(int *)(segments[num]->name - 4) = num + 1;
  }
}
 
 
/* This function adds a new segnode to the front of a given list. This is used for the
adjacency list */

addnode(segP,data1,data2)
     AREG1 SEGNODE **segP;
     SEGMENT *data1;
     SEGMENT *data2;
{
  AREG2 SEGNODE *newnode;

  newnode = (SEGNODE *)MALLOC(sizeof(SEGNODE));
  newnode->seg1 = data1;
  newnode->seg2 = data2;
  newnode->next = *segP;
  *segP = newnode;
}


/* This function adds a new item to the front of a given list. This is used for lists
of strings and functions. */

additem(itemlistP,data)
     AREG1 ITEM **itemlistP;
     AREG3 LGSYMBOL *data;
{
  AREG2 ITEM *newnode;

  newnode = (ITEM *)MALLOC(sizeof(ITEM));
  newnode->sym = data;
  newnode->next = *itemlistP;
  *itemlistP = newnode;
}


/* This function outputs the segment file. The file consists of two parts: a list
of each segment, its adjacent segments, and the functions and string it contains is
the first part of the file.  The second part of the file is a list of each function,
and the functions and strings it uses. */

segout(game_name)
     AREG1 char *game_name;
{
  DREG1 int i;
  DREG2 int j;
  AREG3 ITEM *tempitem;
  char *filename;
  int fnamelen;

  if (segments == NULL) {
    my_printf("No segments defined.\n");
    return; }
 
  if (segments[0] == NULL) {
    my_fprintf(stderr,"Error: Must have segment named '0'.\n");
    my_exit(1);
  }
  fnamelen = strlen(game_name);
  filename = MALLOC(fnamelen + 5);
  strcpy(filename,game_name);
  strcat(filename,".seginfo");  /* Get the correct file name and open file */
  if ((segfd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0666)) < 0) {
    my_fprintf(stderr, "ZAP: Can't open segment info file \"%s\"\n",filename);
    my_exit(SCNOFILE);
  }
  segbufs[segbuf] = MALLOC(SEGBSIZ); /* Initialize the buffers */
  adjtable = setadj(adjacents);      /* Make the adjacency table */
  segput("SEGMENT LIST\n");
  for(i = 0; i < numsegs; i++) {   /* Output each segment */ 
    segput("Segment name: ");
    segput(segments[i]->name);
    segput("\n\nAdjacent segments :\n\n");
    for(j = 0; j < numsegs; j++) /* Output adjacent segments */
      if ((adjtable[(i * numsegs + j) / 8] & bit[(i * numsegs + j) % 8]) != 0) {
	segput(segments[j]->name);
	segput("\n");
      }
    segput("\nIncluded functions:\n\n");  /* Included functions and strings */
    putlist(segments[i]->functions);
    segput("\nIncluded strings:\n\n");
    putlist(segments[i]->strings);
    segput("\n\n");
  }
  segput("\nFUNCTION LIST\n");            /* Part 2 of the file; the function list */
  for(tempitem = funlist; tempitem != NULL; tempitem = tempitem->next) {
    segput(tempitem->sym->lg_sym.sym_name);     /* The function's name */
    segput("\nUsed functions and strings:\n");  /* Output used functions and strings */
    putlist(tempitem->sym->lg_add_info.function.calls);
    segput("\n");
  }
  for (i = 0; i < segbuf; i++)   /* Now write out the buffers. */
    if (write(segfd,segbufs[i],SEGBSIZ) != SEGBSIZ) {
      my_fprintf(stderr,"ZAP: Error writing SEGINFO file.\n");
      my_exit(SCERR);
    }
  if (segloc != 0)
    if (write(segfd,segbufs[segbuf],segloc) != segloc) {
      my_fprintf(stderr,"ZAP: Error writing SEGINFO file.\n");
      my_exit(SCERR);
    }
  if (close(segfd) < 0) {
    my_fprintf(stderr,"ZAP: Cannot close SEGINFO file.\n");
    my_exit(SCNOFILE);
  }
  free(filename);
  segfile(game_name);   /* Now output the .seg file */
}


/* This function puts a list of functions and/or strings into the .seginfo file */

putlist(list,startstr)
  AREG1 ITEM *list;
  AREG2 char *startstr;
{
  ITEM *tempitem;

    for(tempitem = list; tempitem != NULL; tempitem = tempitem->next) {
      segput(tempitem->sym->lg_sym.sym_name);  /* Output the name... */
      segput(" ");
      segput(itoa(tempitem->sym->lg_val.v_value)); /* and the value */
      segput("\n");
    }
}


/* This function puts an item into the .seg or .seginfo file */
segput(thing_ptr)
  AREG1 char *thing_ptr;
{	
  DREG1 int i;

	for (i = 0; thing_ptr[i] != '\0'; i++) {
	  segbufs[segbuf][segloc++] = thing_ptr[i];
	  if (segloc == SEGBSIZ) {
	    if (++segbuf == SEGBNUM) {
	      my_fprintf(stderr,"Segment file too big; increase constant SEGBNUM.\n");
	      my_exit(SCERR);
	    }
	    if (segbufs[segbuf] == NULL)
	      segbufs[segbuf] = MALLOC(SEGBSIZ);
	    segloc = 0;
	  }
	}
}


/* This function puts an item of given length into the .seg or .seginfo file */ 
segputl(thing_ptr,thing_len)
  AREG1 char *thing_ptr;
  DREG1 int thing_len;
{	
  DREG1 int i;

	for (i = 0; i < thing_len; i++) {
	  segbufs[segbuf][segloc++] = thing_ptr[i];
	  if (segloc == SEGBSIZ) {
	    if (++segbuf == SEGBNUM) {
	      my_fprintf(stderr,"Segment file too big; increase constant SEGBNUM.\n");
	      my_exit(SCERR);
	    }
	    if (segbufs[segbuf] == NULL)
	      segbufs[segbuf] = MALLOC(SEGBSIZ);
	    segloc = 0;
	  }
	}
}

static char strbuf[50];  /* This buffer is used by itoa */

/* This turns an integer into an ascii string. It's from K & R. */
char
*itoa(n)
DREG3 long n;
{
  AREG1 char *s;
  DREG1 int i = 50;
  DREG2 int sign;

  s = strbuf;
  s[--i] = '\0';
 
  if ((sign = n) < 0)
    n = -n;
  do {
    s[--i] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    s[--i] = '-';
  return(&s[i]);
}


/* This procedure outputs the .seg file, which is read by the zsplit program.  The
   file format is as follows:
        # of segments                      (2 bytes)
	Last page of pure tables (ENDLOD)  (2 bytes)
	First page of function space       (2 bytes)
	Last page of object code           (2 bytes)
	Adjacency bit table
	Segment #0 name
	Segment #0 used pages bit table
	Segment #1 name
	Segment #1 used pages bit table
	...
	Segment #n name
	Segment #1 used pages bit table
*/
segfile(game_name)
     char *game_name;
{
  char fnm[128];
  DREG1 int i;
  AREG1 LGSYMBOL *sym;
  long seg0_data = funct_start + 255;

  strcpy(&fnm[0], game_name);
  strcat(&fnm[0], ".seg");
  /* Now do the actual segment file. */
  if ((segfd = open(&fnm[0],O_WRONLY | O_CREAT | O_TRUNC,0666)) < 0) {
    my_fprintf(stderr, "ZAP: Can not open segment file \"%s\"\n",&fnm[0]);
    my_exit(SCNOFILE);
  }

  segloc = segbuf = 0;  /* Let's reuse the buffers */
  segputl((char *)&numsegs,sizeof(UWORD)); /* Number of segments */
  if ((sym = symlookup(Gblsymtab,ENDLODSYM)) == NULL) {  /* Find the ENDLOD symbol */
    zerror(E_ALWAYS, "No %s symbol.",ENDLODSYM);
    return(SCERR);
  }
  endlod_page = sym->lg_val.v_value / PAGE_SIZE; /* Endload page */
  /* Everything from the start of the file to function_space goes in segment 0 */
  if ((sym = symlookup(Gblsymtab, "SEG-DATA")) != NULL) {
    seg0_data = sym->lg_val.v_value;
    my_printf("Data from %d to %d not in segment 0.\n", seg0_data, funct_start + 256); }
  addpages(0, seg0_data / PAGE_SIZE, segments[0]->pages); 
  funct_page = (funct_start + 256) / PAGE_SIZE;  /* First function page */
  segputl(&endlod_page,sizeof(UWORD)); /* Output words */
  segputl(&funct_page,sizeof(UWORD));
  segputl(&top_page,sizeof(UWORD));
  segputl(adjtable,(numsegs * numsegs + 7) / 8);  /* Output the adjacency table */
  for (i = 0; i < numsegs; i++) {
    setpages(segments[i]);      /* Set the page table for a segment */
    segput(segments[i]->name);  /* Output the name... */
    segputl("\0",1);
    segputl(segments[i]->pages,NUM_PAGES / 8); /* And the page table */
  }

  for (i = 0; i < segbuf; i++)    /* Write out the buffers */
    if (write(segfd,segbufs[i],SEGBSIZ) != SEGBSIZ) {
      my_fprintf(stderr,"ZAP: Error writing SEG file.\n");
      my_exit(SCERR);
    }
  if (segloc != 0)
    if (write(segfd,segbufs[segbuf],segloc) != segloc) {
      my_fprintf(stderr,"ZAP: Error writing SEG file.\n");
      my_exit(SCERR);
    }
  if (close(segfd) < 0) {
    my_fprintf(stderr,"ZAP: Cannot close SEG file.\n");
    my_exit(SCNOFILE);
  }
}


/* This adds a sequence of pages to a page bit table */
addpages(start,end,table)
           UWORD start;
           UWORD end;
     AREG1 char table[];
{
  DREG1 int i;

  for (i = start; i <= end; i++)
    table[i / 8] |= bit[i % 8];
}


/* This sets the page bit table for a given segment */
setpages(seg)
     AREG1 SEGMENT *seg;
{
  AREG2 ITEM *temp;
  segment_marker *chain = seg->segs;
  long epc, bpc;
  
  for(temp = seg->strings; temp != NULL; temp = temp->next)  /* Do all strings... */
    addpages(temp->sym->lg_add_info.string.start_page,
	     temp->sym->lg_add_info.string.end_page,
	     seg->pages);
  funpages(seg->functions,seg);                              /* and functions */
  /* And tables, if any */
  while (chain) {
    bpc = chain->beginning_pc;
    epc = chain->ending_pc / PAGE_SIZE;
    if ((bpc < (funct_start + 256)) && (epc > endlod_page)) {
      /* Skip anything that's not in pure table space */
      bpc /= PAGE_SIZE;
      if (bpc <= endlod_page) bpc = endlod_page + 1;
      addpages(bpc, epc, seg->pages); }
    chain = chain->next; }
}


/* This goes through a function's call tree, setting the bit table as it goes
   along */
funpages(funs,seg)
           ITEM *funs;
     AREG2 SEGMENT *seg;
{
  AREG3 ITEM *temp;

  for(temp = funs; temp != NULL; temp = temp->next) /* Do each function in list */
    if (temp->sym->lg_space == FUNCTION_SPACE) {
      if (temp->sym->lg_add_info.function.temp.lastseg != seg) { /* Only do it once */
	temp->sym->lg_add_info.function.temp.lastseg = seg;
	addpages(temp->sym->lg_add_info.function.start_page, /* add pages... */
		 temp->sym->lg_add_info.function.end_page,
		 seg->pages);
	funpages(temp->sym->lg_add_info.function.calls,seg); /* Call recursively */
      } 
    } else if (temp->sym->lg_space == ZSTRING_SPACE)
      addpages(temp->sym->lg_add_info.string.start_page,   /* Do the string */
	       temp->sym->lg_add_info.string.end_page,
	       seg->pages);
}


/* This sets up the adjacency bit table, taking the information from the adjacent
   segment linked list. Note that each pair is represented in the table by two 
   different locations. */
char
*setadj(adjs)
     AREG1 SEGNODE *adjs;
{ 
  AREG2 SEGNODE *temp;
  int loc1,loc2;
  AREG3 char *table;

  table = CALLOC(1,(numsegs * numsegs + 7) / 8);
  for (temp = adjs; temp != NULL; temp = temp->next) {
    loc1 = getnum(temp->seg1->name) * numsegs + getnum(temp->seg2->name);
    loc2 = getnum(temp->seg2->name) * numsegs + getnum(temp->seg1->name);
    table[loc1 / 8] |= bit[loc1 % 8];
    table[loc2 / 8] |= bit[loc2 % 8];
  }
  return(table);
}


/* segstr( strP )

	Return pointer to segment name string pointer

Accepts :

	strP		String to reference

Returns :

	<value>		Pointer to ptr to a single copy of the string.
	This is just like the literal string routine in zlex.c, but the table only
contains segment names.

*/ 
char **
segstr( strP )
	char		*strP;
{
	SYMBOL		*symP;		/* Symbol table ptr */

    /* Make or reference an entry in the segment table */
    symP = symenter( Segsymtab, strP, sizeof( SYMBOL ) );
    return( &(symP->sym_name) );
}


