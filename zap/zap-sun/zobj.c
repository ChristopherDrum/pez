/*	zobj.c		Object file handling

	1988 Zinn Computer Company, for Infocom


	This file contains code that handles data for the object file.

	Object file data is buffered in memory until it needs to be
written.  The data is kept in fixed size blocks that are pointed to by
an array of pointers to them.  All this information is private to this
module.  There are several reasons for this arrangement of data, but
mainly it is to help manage the object file in memory, across different
architectures (segmented vs nonsegmented) as well as dealing with
unix malloc() in a friendly way.  This module also ensures correct
byte order of non-byte values.

Most of the routines here are no-ops if the build_freq flag is set, since
in that case we aren't building an object file.

Contained in this file:

	objalign	Align to n-byte boundary
	objclose	[Write and] Close the object file
	objopen		Open the object file && init object handling
	objputb		Put a byte into the object stream
	objputw		Put a word into the object stream
	objputzstr	Put a Z-string into the object stream
 	objput		Put data in into the object stream
	objseek		Seek within the object file
	objtell		Tell where we are in the object file
	objvocab	Output a vocabulary tree node (and its children)
	voclabel	Associate a label with the current vocab entry

And internal routine(s):

	addvbyte	Add a byte to vocabulary info
	insvnode	Insert node to vocabulary tree

*/

#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern	char	*CALLOC();
extern	char	*REALLOC();

/* External data */

extern	int	build_freq;
extern	UWORD	State;
extern	int	Vocreclen;
extern 	int Vockeylen;
extern	VNODE	*Voctrees[];

extern	long	S_objdata;
extern	long	S_objsize;

extern  BOOL    yoffsets;
extern  UWORD   top_page;
extern	VNODE	*CurvocP;
extern	int	VocX;

extern	FIXUP	*glo_chain;

/* Local routines (and forward references) */

	long	objtell();

/* Local data */


/* Private data */

static	int	Objfd;		/* File descr for object file */
extern	long	Objtop;		/* Top of object file */
static	UBYTE	**Oblist;	/* Ptrs to buffers */
static	int	Oblmax = 0;	/* How many slots there are in Obblist */
static	int	Oblnum = 0;	/* How many buffers we have */
static	int	OblX;		/* Which buffer we're using */
static	UBYTE	*ObP;		/* Ptr to current buffer */
static	int	ObX;		/* Index in current buffer */
/*
 
*//* objalign( alignment )

	Align to n-byte boundary

Accepts :

	alignment	Byte factor for alignment

Returns :

	<value>		Status code

*/

int
objalign( alignment )
DREG3	int		alignment;
{
DREG2	int		sts;
DREG1	int		n;
    if (build_freq) return(SCOK);
    n = (int)(objtell() % alignment);
    if ( n != 0 )
	while( n++ < alignment )
	    if ( ( sts = objputb( 0 ) ) != SCOK )
		return( sts );

    return( SCOK );
}
/*
 
*//* objopen( filename )

	Opens the object file and inits this module

Accepts :

	filename	Name of file to open

Returns :

	<value>		Status code

*/

int
objopen( filename )
	char		*filename;	/* Name of file to open */
{
    if ( ( Objfd = open( filename, O_WRONLY|O_CREAT|O_TRUNC, 0666 ) ) < 0 ) {
	my_fprintf( stderr, "Can not open object file \"%s\"", filename );
	return( SCNOFILE );
    }

    /* Setup the buffer system */
    Oblmax = (int)(262144L/OBBSIZE);
    if ( ( Oblist = (UBYTE **)CALLOC( Oblmax, sizeof(UBYTE **) ) ) == NULL ) {
	my_fprintf( stderr, "Can't allocate Oblist in objopen()\n" );
	close( Objfd );
	return( SCMEMORY );
    }

    Oblnum = 0;			/* No buffers yet */
    OblX = -1;			/* No index... */

    return( objseek( 0L, TRUE ) );
}
/*
 
*//* objfinish()
	Writes the checksum and length to the file, and possibly start of string
	space and function space.
*/

objfinish()
{
	UWORD length;
  DREG1 UWORD checksum;
  DREG2 long count;
  DREG3 int sts;
	int div_val;

	if (yoffsets)
	  div_val = 8;
	else
	  div_val = 4;
	if ((sts = objalign(div_val)) != SCOK)
		return(sts);
	objtell();
	top_page = Objtop / PAGE_SIZE;
	if (top_page > NUM_PAGES - 1) {
	  zerror(E_ALWAYS,"Uh Oh!  Game is too big!");
	  return(SCERR);
	}
	length = Objtop / div_val;
	checksum = 0;
	for (count = 64; count <= Objtop; count++)
		checksum += Oblist[count / OBBSIZE][count % OBBSIZE];
	if ((sts = objseek(OBJ_LEN_LOC,FALSE)) != SCOK)
		return(sts);
	if ((sts = objputw(length)) != SCOK)
		return(sts);
	if ((sts = objseek(OBJ_CHK_LOC,FALSE)) != SCOK)
		return(sts);
	if ((sts = objputw(checksum)) != SCOK)
		return(sts);
	return(set_serial());
}

set_serial()
{
  int sts;
  struct tm *tms;
  int ct;
  unsigned char buf[10];
  long tt = time(0);
  tms = localtime(&tt);
  if ((sts = objseek(OBJ_SERIAL_LOC, FALSE)) != SCOK)
    return(sts);
  sprintf(&buf[0], "%02d%02d%02d", tms->tm_year, tms->tm_mon + 1,
	  tms->tm_mday);
  for (ct = 0; ct < OBJ_SERIAL_LEN; ct++) {
    if ((sts = objputb(buf[ct])) != SCOK)
      return(sts); }
  return(sts); }


/* objclose(wrtF)

	Close the object file

Accepts :

	wrtF		Whether to keep the object data

Returns :

	<value> 	Status code

*/

int
objclose( wrtF )
	int		wrtF;		/* Whether to keep */
{
	int		i;
	int		sts;
	long		wrtC;		/* Write count */
	UWORD		ioC;		/* I/O count */

    sts = SCOK;				/* Assume success */

    if ( wrtF ) {
        objtell();			/* To calculate Objtop */

	for( i = 0, wrtC = 0L; wrtC < Objtop ; ++i ) {
	    ioC = (unsigned)OBBSIZE;	/* How much to write */
	    if ( ioC > ( Objtop - wrtC ) )
		ioC = Objtop - wrtC;
	    if ( write( Objfd, Oblist[i], ioC ) != ioC ) {
		my_fprintf( stderr, "write failed to object file objclose()\n" );
		sts = SCEOF;
		break;
	    }
	    wrtC += ioC;
	}
	S_objdata = wrtC;
	lseek(Objfd, 0L, 2);
	wrtC = lseek(Objfd, 0L, 1);
	i = wrtC % PAGE_SIZE;
	/* Fill to a page boundary */
	if (i > 0) {
	  char foo = 0;
	  i = PAGE_SIZE - i;
	  while (i-- > 0)
	    write(Objfd, &foo, 1); }
	S_objdata = lseek(Objfd, 0L, 1);
    }

    close( Objfd );
    return( sts );
}
/*
 
*//* objput( dataP, datalen )

	Output data to the object file

Accepts :

	dataP		Address of the data
	datalen		Number of bytes to output


Returns :

	<value>		Status code

*/

int
objput( dataP, datalen )
AREG1	UBYTE		*dataP;		/* Data address */
DREG2	int		datalen;	/* Number of bytes of data */
{
DREG1	int		sts;
    if (build_freq) return(SCOK);
    while( datalen-- > 0 )
	if ( ( sts = objputb( *dataP++ ) ) != SCOK )
	    return( sts );

return( SCOK );
}
/*
 
*//* objputb( b )

	Put a byte value into the object file

Accepts :

	b		The byte to output

Returns :

	< value >	Status code

*/

int
objputb( b )
	int		b;		/* Really a byte */
{
	int		sts;
    if (build_freq) return(SCOK);
    /* Intercepting vocabulary data? */
    if ( ( State & AS_VOCAB ) != 0 )
	return( addvbyte( b ) );	/*  redirect to vocab tree */
    else if ( ObX == OBBSIZE )		/* Need another buffer ? */
	if ( ( sts = objseek( objtell(), FALSE ) ) != SCOK )
	    return( sts );

    ObP[ObX++] = b;			/* Store the value */
    return( SCOK );
}

int objgetb()
{
  int sts;
  if (build_freq) return(0);
  if ((State & AS_VOCAB) != 0) {
    my_printf("Get from vocabulary area attempted.\n");
    exit(1); }
  if (ObX == OBBSIZE)
    if ((sts = objseek(objtell(), FALSE)) != SCOK) {
      my_printf("GETB failed.\n");
      exit(1); }
  return(ObP[ObX++]); }

/*
 
*//* objputw( w )

	Put a word value into the object file

Accepts :

	w		The word to output

Returns :

	< value >	Status code

*/

int
objputw( w )
	UWORD		w;		/* Word to output */
{
	UBYTE		b1, b2;		/* The two bytes */
	int		sts;
    if (build_freq) return(SCOK);
    b1 = w >> 8;
    b2 = w & 0xff;

    /* If redirecting to vocabulary tree, or
       if spanning a buffer boundary, just hand off to objputb() */
    if ( ( ( State & AS_VOCAB ) != 0 ) ||
         ( ObX >= (OBBSIZE-1) ) ) {
	if ( ( sts = objputb( b1 ) ) == SCOK )
	    sts = objputb( b2 );
	return( sts );
    }

    /* We can do it here... */
    ObP[ObX++] = b1;			/* Store top byte */
    ObP[ObX++] = b2;			/* Store low byte */
    return( SCOK );
}
/*
 
*//* objputzstr( zstrP )

	Put a z-string out to the object file

Accepts :

	zstrP		Address of string (5-bit-byte encoding)

Returns :

	< value >	Status code

*/

int
objputzstr( zstrP )
AREG1	UBYTE		*zstrP;
{
DREG3	int		sts;
DREG1	UBYTE		b1;
    if (build_freq) return(SCOK);
    /* Output bytes until end of zstring (word with high bit set) */
    do {
	b1 = *zstrP++;
	if ( ( sts = objputb( b1 ) ) == SCOK )
	    sts = objputb( *zstrP++ );
    } while ( ( ( b1 & 0x80 ) == 0 ) && ( sts == SCOK ) );

    return( sts );
}
/*
 
*//* objseek( newpos, truncF )

	Seek to a position in the object file

Accepts :

	newpos		Position to seek to
	truncF		Whether new position becomes the top if it
			  is lower than current top.

Returns :

	<value>		Status code

*/

int
objseek( newpos, truncF )
	long		newpos;		/* Where to seek to */
	int		truncF;		/* Whether to truncate */
{
	int		blkno;		/* Block number */
	int		blkoffs;	/* Block offset */
	UBYTE           **oldOblist;
	int             oldOblmax
;
    if (build_freq) return(SCOK);
    blkno = (int)(newpos/OBBSIZE);	/* New block number */
    blkoffs = (int)(newpos % OBBSIZE);	/* New block offset */

    /* Expand the pointer list if we have to */
    if ( blkno >= Oblmax ) {
        oldOblist = Oblist;
	oldOblmax = Oblmax;
	Oblmax = blkno + 20;		/* Some large expansion */
	Oblist = (UBYTE **)CALLOC( Oblmax, sizeof(UBYTE **) );
	if ( Oblist == NULL ) {
	    my_fprintf( stderr, "Can't reallocate Oblist in objseek()\n" );
	    return( SCMEMORY );
	}
	memcpy(Oblist,oldOblist,oldOblmax * sizeof(UBYTE **));
	CFREE(oldOblist,oldOblmax,sizeof(UBYTE**));
    }

    /* Allocate blocks if we have to */
    while( blkno >= Oblnum ) {
	if ( ( Oblist[Oblnum] = (UBYTE *)CALLOC( OBBSIZE, 1 ) ) == NULL ) {
	    my_fprintf( stderr, "Can't allocate buffer in objsect()\n" );
	    return( SCMEMORY );
	}
	S_objsize += OBBSIZE;
	++Oblnum;
    }

    /* Set new position */
    objtell();				/* Make sure Objtop is right */
    OblX = blkno;
    ObP = Oblist[OblX];
    ObX = blkoffs;

    if ( truncF && ( newpos < Objtop ) )
	Objtop = newpos;

    return( SCOK );
}
/*
 
*//* objtell()

	Return current position in object file

Accepts :

Returns :

	<value>		Current position.

Notes :

	Side-effect is to update Objtop if we're past that value.

*/

long
objtell()
{
DREG1	long		filepos;
    if (build_freq) return(0);

    filepos = (long)OblX * OBBSIZE;
    filepos += ObX;

    if ( filepos > Objtop )
	Objtop = filepos;

    return( filepos );
}
/*
 
*//* objvocab( nodeP )

	Output a vocabulary node and its children

Accepts :

	nodeP		Ptr to node to output

Returns :

	<value>		Status code

Notes :

	This routine also frees the memory occupied by the node
and its children.

	If a partially constructed node is still hanging around,
an error message is given and that node is freed.

This procedure has been modified.  It now takes an array of trees and runs objvoc --
what was previously called objvocab -- on each array element.

*/

int
objvocab(nodes)
     AREG1 VNODE *nodes[];
{
  DREG1 int i;

  /* Check for partially filled node not yet inserted */
  if ( CurvocP != NULL ) {
    zerror( E_PASS1, "warning - partial vocabulary data discarded" );
    CFREE( CurvocP,1,sizeof(VNODE) + Vocreclen - 1 );
    CurvocP = NULL;
    VocX = 0;
  }
  for(i = -1; i < 255; objvoc(Voctrees[++i]));
}


int
objvoc( nodeP )
AREG2	VNODE		*nodeP;		/* Addr of vocab node */
{
AREG1	VLABEL		*vlP;		/* Label block data */
AREG3	LGSYMBOL	*symP;
FIXUP *vf;
    if (build_freq) return(SCOK);
    if ( nodeP != NULL ) {		/* Proceed if any node to output */
	objvoc( nodeP->vn_leftP );	/* Do the left branch */

	/* Take care of any labels associated with this point */
	while( ( vlP = nodeP->vn_labelP ) != NULL ) {
	    nodeP->vn_labelP = vlP->vl_nextP; /* Unlink it */
	    symP = vlP->vl_symP;	/* Get symbol address */
	    symP->lg_loc = objtell() + vlP->vl_offset;
	    symP->lg_val.v_value = (ZNUM)symP->lg_loc;
	    CFREE( vlP, 1, sizeof(VLABEL) );
	}
	while ((vf = nodeP->vn_fixups) != NULL) {
	  /* Process any forward references for this node, by adding them
	     to the global fixup chain with the appropriate address (which
	     we now know. */
	  nodeP->vn_fixups = vf->fx_next;
	  vf->fx_addr += objtell();
	  vf->fx_next = glo_chain;
	  glo_chain = vf; }
	    
	/* Output the data for this vocab node */
	objput( &nodeP->vn_data[0], Vocreclen );

	objvoc( nodeP->vn_rightP );	/* Do the right branch */
	CFREE( nodeP, 1, sizeof(VNODE) + Vocreclen - 1 ); /* Get rid of this node */
    }

    return( SCOK );
}
/*
 
*//* addvbyte( b )

	Insert a byte into the vocabulary tree

Accepts :

	b		The new byte of vocabulary info

Returns :

	<value>		Status code

Notes :

	A new node is created for the tree when the first
byte for the node comes in.  The node is then inserted into the
tree when it becomes full.

*/

static int
addvbyte( b )
	UBYTE		b;
{
DREG1	int		sts;
    if (build_freq) return(SCOK);
    /* Upon the first byte, make a new node */
    if ( ( VocX == 0 ) && ( CurvocP == NULL ) ) {
	CurvocP = (VNODE *)CALLOC( 1, sizeof(VNODE) + Vocreclen - 1);
	if ( CurvocP == NULL ) {
	    zerror( E_ALWAYS, "can not allocate vocabulary tree node!" );
	    return( SCMEMORY );
	}
    }

    /* Add the data to the node */
    CurvocP->vn_data[VocX++] = b;

    /* If the node is now full, add it to the tree */
    if ( VocX == Vocreclen ) {
	sts = insvnode( CurvocP );
	CurvocP = NULL;
	VocX = 0;
    }
    else
	sts = SCOK;

    return( sts );
}
/*
 
*//* insvnode( nodeP )

	Insert node into vocabulary tree

Accepts :

	nodeP		Node to insert

Returns :

	<value>		Status code

*/

static int
insvnode( nodeP )
AREG1	VNODE		*nodeP;
{
DREG1	int		keylen;		/* Length of comparison key */
AREG2	VNODE		**linkPP;	/* Ptr to insertion link */
AREG3	VNODE		*nnodeP;	/* Ptr to next node */
    if (build_freq) return(SCOK);
    /* Get length of comparison key, in bytes */
    keylen = Vockeylen;

    /* Find insertion point */
    for( linkPP = &Voctrees[(unsigned char)nodeP->vn_data[0]];
	                                           ( nnodeP = *linkPP ) != NULL; ) {
	/* Find which branch to descend */
	if ( reccmp( &nodeP->vn_data[0], &nnodeP->vn_data[0], keylen ) < 0 )
	    linkPP = &nnodeP->vn_leftP;
	else
	    linkPP = &nnodeP->vn_rightP;
    }

    /* Got insertion point */
    *linkPP = nodeP;

    return( SCOK );
}
/*
 
*//* voclabel( symP )

	Associate a label with the current vocabulary entry position

Accepts:

	symP		Ptr to the symbol block for the label

Returns :

	<value>		status code

*/

int
voclabel( symP )
AREG1	LGSYMBOL	*symP;		/* Ptr to the symbol table entry */
{
DREG1	int		sts;
AREG2	VLABEL		*vlP;		/* Label block */

    /* If no current vocab node, construct a node by pretending to
       add a byte... */
    if (build_freq) return(SCOK);
    if ( CurvocP == NULL ) {
	if ( ( sts = addvbyte( 0 ) ) != SCOK )
	    return( sts );
	--VocX;				/* Take away the byte */
    }

    /* Make a label node for this label */
    if ( ( vlP = (VLABEL *)CALLOC( 1, sizeof(VLABEL) ) ) == NULL ) {
	zerror( E_ALWAYS, "can not allocate vocabulary label node!" );
	return( SCMEMORY );
    }

    /* Construct the label info and link it in */
    vlP->vl_symP = symP;
    vlP->vl_offset = VocX;

    vlP->vl_nextP = CurvocP->vn_labelP;
    CurvocP->vn_labelP = vlP;

    return( SCOK );
}


reccmp(r1,r2,len)
     DREG1 unsigned char *r1;
     DREG2 unsigned char *r2;
     AREG1 int len;
{
	for (;*r1 == *r2; r1++, r2++)
		if (--len == 0)
			return(0);
	return(*r1 - *r2);
}
		
