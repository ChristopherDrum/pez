/*	zlex.c		Source tokenizer/lexicator

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	tklex		Tokenize and lexicalize an input file

Also, internal routines:

	lexadd		Add lexical entry to token stream
	lexdirop	Process directive or opcode
	lexlinemark	Insert line marker
	gettoken	Get a token
*/

#include <stdio.h>
#include <ctype.h>

#include "zap.h"


/* Local definitions */

#define __SEG__ Seg2

/* External routines */

extern  int	my_fprintf();
extern	long	atol();
extern	FILE	*fopen();
extern	long	ftell();
extern	SYMBOL	*symenter();
extern	SYMBOL	*symlookup();
extern 	char	*MALLOC();
extern 	char	*CALLOC();
extern 	char	*REALLOC();

/* External data */

extern	LEXBUF	*LexH;			/* Lex buffer header */
extern	LEXBUF	*LexP;			/* Current lex buffer pointer */
extern	UBYTE	*LextkP;		/* Lex token pointer */
extern	STABLE	*Litsymtab;
extern	STABLE	*Odsymtab;
extern	BOOL	Opsrcerr;

extern	long	S_lexmax;
extern	long	S_lexttl;
extern	long	S_lexsize;
extern	long	S_srcsize;
extern	int		Max_buf_len;

/* Local routines (and forward references) */

	char	**litstr();
	char	*get_buffer();
	char	*expand_buffer();

/* Local data */


/* Private data */

static	int	Insertlevel;
static	UBYTE	*LasttkP;
/*
 
*//* tklex( lxPP, lxXP, filename )

	Tokenize and interpret tokens from source


Accepts :

	lxPP		Address of variable pointing to LEXBUF structure.
			This may be NULL if none is currently allocated.
	lxXP		Address of variable that has the next available
			 index into the LEXBUF data.

Returns :

	< value >	Status code.
	*lxPP, *lxxP	Updated to reflect where tokenizing left off.
	LexH		Set to LEXBUF at head of list, if it needs to be.

*/

char curfile[STRINGBUF+1];

int
tklex( lxPP, lxXP, filename )
	LEXBUF		**lxPP;		/* Current buffer input/output */
	int		*lxXP;		/* Buffer index input/output */
	char		*filename;	/* Name of file to scan */
{
	int		c;
	ZNUM		val;		/* Scratch value */
DREG1	int		sts;
DREG2	int		tktype;		/* Type of token */
	int		eolF;		/* End-of-line flag */
	int		bolF;		/* Beginning-of-line flag */
	int		linenum;	/* Line counter */
	int		lxX;		/* Current data index */
AREG2	char		*strP;		/* Random string pointer */
	LEXBUF		*lxP;		/* Current lex buf */
AREG1	FILE		*fP;		/* For source file */
	char		*token; /* New token */

	token = get_buffer();

    strcpy( &curfile[0], filename );

	if ( ( fP = fopen( &curfile[0], "r" ) ) == NULL ) {
	  strcat( &curfile[0], ".zap" );
	  if ( ( fP = fopen( &curfile[0], "r" ) ) == NULL ) {
	    sprintf(&curfile[0], "%s.xzap", filename);
	    if ((fP = fopen(&curfile[0], "r")) == NULL) {
	      my_fprintf( stderr, "ZAP: Can't open \"%s\"\n", filename );
	      return( SCNOFILE ); } } }
	my_fprintf(stderr, "%6d  Inserting %s.\n", objtell(), &curfile[0]);


    if ( ( lxP = *lxPP ) == NULL ) {
	/* Need to initialize the LEXBUF list */
	LexH = lxP = (LEXBUF *)MALLOC( sizeof(LEXBUF) );
	if ( lxP == NULL ) {
	    fclose( fP );
	    my_fprintf( stderr, "ZAP: Can't allocate LEXBUF(1) in tklex()\n" );
	    return( SCMEMORY );
	}

	S_lexttl = S_lexsize = S_lexmax = sizeof(LEXBUF);

	lxX = 0;

	/* Also setup the assembly point */
	LexP = LexH;
	LextkP = &(LexP->lx_data[0]);
    }
    else
	lxX = *lxXP;

    /* Place the file reference token in the stream */
    sts = lexadd( &lxP, &lxX, TKFILE, &curfile[0], strlen(&curfile[0]) +1 );
    if ( sts != SCOK ) {
	fclose( fP );
	return( sts );
    }

    /* Go through and process tokens */
    eolF = TRUE;
    linenum = 1;			/* Line 1 */
    for ( ; ; ) {
	/* If line was ended, begin new line.  Note that this may
	   generate some extra line marks. */
	if ( eolF ) {
	    if ( ( sts = lexlinemark( &lxP, &lxX, linenum, fP ) ) != SCOK )
		break;
	    bolF = TRUE;
	    eolF = FALSE;
	}

	if ( ( tktype = gettoken( fP, &token, bolF ) ) == TKEOF )
	    break;

	switch( tktype ) {		/* Process by token type */
	    case TKCHAR:		/* Some random character */
		sts = lexadd( &lxP, &lxX, TKCHAR, &token[0], 1 );
		break;

	    case TKLABEL:		/* Beginning of line stuff */
	    case TKGLABEL:
	    case TKEQUATE:
	        /* Add the token... */
		sts = lexadd( &lxP, &lxX, tktype,
				   litstr( &token[0] ), sizeof(char *) );
		break;


	    case TKSYMBOL:		/* Some symbol */
		/* If appropriate, check for directive or opcode */
		if ( bolF ) {
		    bolF = FALSE;
		    sts = lexdirop( &lxP, &lxX, &token[0], fP, &curfile[0] );
		    if ( sts != SCNOTDONE )
			break;		/* It was handled! */
		}

		/* Nothing special */
		sts = lexadd( &lxP, &lxX, tktype,
				   litstr( &token[0] ), sizeof(char *) );
		break;


	    case TKSTRING:		/* Token that's some string */
		/* For a string, count the # of lines it takes up */
		for( strP = &token[0]; (c = *strP++) != NUL; )
		    if ( c == '\n' )
			++linenum;
		/* Drop through to add a string */

	    case TKOTHER:
		sts = lexadd( &lxP, &lxX, tktype, &token[0],
				strlen( &token[0] ) +1 );
		break;

	    case TKINTEGER:
	        val = (ZNUM)atol( &token[0] );
		sts = lexadd( &lxP, &lxX, tktype, &val, sizeof(ZNUM) );
		break;

	    case TKEOL:			/* End of source line */
		eolF = TRUE;
		++linenum;
		break;

	    default:
		sts = lexadd( &lxP, &lxX, tktype, &token[0], 0 );
		break;
	}

	if ( sts != SCOK )
	    break;
    }

    if ( ( sts == SCOK ) && ( tktype == TKEOF ) && ( Insertlevel == 0 ) )
	sts = lexadd( &lxP, &lxX, TKEOF, &token[0], 0 );

    S_srcsize += ftell( fP );

    fclose( fP );

    /* Update the caller's pointers */
    *lxPP = lxP;
    *lxXP = lxX;

	free_buffer();
    return( sts );
}
/*
 
*//* lexdirop( lxPP, lxXP, tokenP, fP, filename )

	Check for directive or opcode

Accepts :

	lxPP		Address of ptr to LEXBUF in use
	lxXP		Address of lex buf index variable
	tokenP		The token to check
	fP		Input file pointer
	filename	Name of current file

Returns :

	<value>		Status code.  SCNOTDONE means not a directive;
			  SCOK means directive or opcode found.  Other
			  status value have usual meaning.
	*lxPP, *lxXP	Updated as necessary
		
*/

int lexdirop(lxPP, lxXP, tokenP, fP, filename)
LEXBUF		**lxPP;		/* Ptr to lexbuf ptr var */
int		*lxXP;		/* lexbuf index */
char		*tokenP;	/* Addr of token */
FILE		*fP;		/* Source file */
char		*filename;	/* Current filename */
{
  UWORD		odnum;		/* Opcode/directive number */
  int		tktype;
  int		sts;
  DREG1	int		flags;
  AREG1	ODSYMBOL	*odsymP;	/* Symtab entry ptr */	
  char		*token;
  char		tch;
  char		*ttok;
  token = get_buffer();
  /* Try to lookup the symbol in the opcode/directive table */
  if ((odsymP = (ODSYMBOL *)symlookup(Odsymtab, tokenP)) == NULL) {
    free_buffer();
    return(SCNOTDONE); }

  /* Found a directive or opcode.  Process! */
  flags = odsymP->od_val.v_flags;
  odnum = odsymP->od_val.v_value;
  
  if ( ( flags & ST_DIRECTIVE ) == 0 ) {
    
    /* It's an opcode.  Translates to 16-bit opcode number. */
    free_buffer();
    return( lexadd( lxPP, lxXP, TKOPCODE, &odnum, sizeof(UWORD) ) ); }

  /* Directive.  Check for special handling */
  if ( ( ( flags & (ST_INSERT|ST_ENDI) ) == 0 ) ||
      ( ( Insertlevel == 0 ) && ( ( flags & ST_ENDI ) != 0 ) ) ) {
    /* Put 8-bit directive number in token stream. */
    token[0] = odnum;
    sts = lexadd( lxPP, lxXP, TKDIRECTIVE, &token[0], 1 );
    free_buffer();
    return( sts ); }
	
  /* Special handling required */
  if ( ( flags & ST_ENDI ) != 0 ) {
    free_buffer();
    return( SCEOF ); }		/* End this insert */

  if ((flags & ST_INSERT) != 0) {	/* Insert another file */
    if ((tktype = gettoken(fP, &token, FALSE)) == TKSTRING) {
      /* Found file name */
      ++Insertlevel;
      ttok = &token[0];
      /* Lower case the file name before proceeding */
      while (tch = *ttok) {
	if ((tch >= 'A') && (tch <= 'Z'))
	  *ttok = tch + ('a' - 'A');
	ttok++; }
      tklex(lxPP, lxXP, &token[0]);
      sts = lexadd(lxPP, lxXP, TKFILE, filename, strlen(filename) + 1);
      --Insertlevel;
      tktype = gettoken(fP, &token, FALSE);
      if ((tktype != TKEOL) && (tktype != TKEOF)) {
	my_fprintf(stderr, "Syntax error in .insert\n");
	sts = SCNOTDONE; } }
    else {				/* Not a string... */
      my_fprintf(stderr, "bad arg for .insert\n");
      sts = SCNOTDONE; }
    while((tktype != TKEOL) && (tktype != TKEOF))
      tktype = gettoken(fP, &token, FALSE);
    ungetc('\n', fP);		/* See EOL next time */
    free_buffer();
    return(sts); }
  free_buffer();
  return( SCNOTDONE ); }
/*
 
*//* lexlinemark( lxPP, lxXP, linenum, fP )

	Insert line marker in token stream

Accepts :

	lxPP		Address of ptr to LEXBUF in use
	lxXP		Address of lex buf index variable
	linenum		Line number
	fP		Input file pointer

Returns :

	<value>		Status code.
	*lxPP, *lxXP	Updated as necessary
		
*/

int
lexlinemark( lxPP, lxXP, linenum, fP )
	LEXBUF		**lxPP;		/* Ptr to lexbuf ptr var */
	int		*lxXP;		/* lexbuf index */
	UWORD		linenum;	/* Line number */
	FILE		*fP;		/* Source file */
{
	int		sts;
	int		tklen;		/* Length of token */
	long		filepos;
	char		databuf[10];

    /* Add the line mark */
    memcpy( &databuf[0], &linenum, tklen = sizeof(UWORD) );
    if ( Opsrcerr ) {			/* If tracking source position */
	filepos = ftell( fP );
	memcpy( &databuf[sizeof(UWORD)], &filepos, sizeof(long) );
	tklen += sizeof(long);
    }

    sts = lexadd( lxPP, lxXP, TKLINE, &databuf[0], tklen );

    /* Do assembly to this point, to minimize buffer space usage */
    while( (sts == SCOK ) && ( LextkP != LasttkP ) )
	sts = asline();

    return( sts );
}
/*
 
*//* gettoken( fP, token_start, bolF )

	Routine to get a token from an input file

Accepts :

	fP		Ptr to FILE thing for input
	token_start		Where to put the token
	bolF		Whether beginning of line

Returns :

	<value>		token type

Notes :

	bolF is checked to see whether token can be a label or a
symbol equate.

*/

int
gettoken( fP, token_start, bolF )
AREG1	FILE		*fP;		/* Ptr to file block */
AREG2	char		**token_start;	/* Where to put token string */
	int		bolF;		/* If beginning of line */
{
DREG2	char		numberF;	/* is a number? */
DREG1	int		c;		/* Character */
DREG3	int		tklen;		/* Lenth of token */
AREG3   char		*tokenP;
	int		token_diff;
        int             buf_siz;

    tokenP = *token_start;
    buf_siz = buffer_size();
    /* Skip over spaces/tabs */
    while( ( c = getc( fP ) ) != EOF )
	if ( ( c != ' ' ) && ( c != '	' ) )
	    break;

    /* Process according to what the character is */
    if ( c == ';' )			/* Flush comments */
	while( ( c = getc( fP ) ) != EOF )
	    if ( c == '\n' )
		break;

    if ( c == EOF )
	return( TKEOF );
    if ( c == '\n' )
	return( TKEOL );

    /* Check for some symbol-like string, or a number */
if ( isdigit(c) || ( c == '-' ) ||
    isalpha(c) || ( c == '.' ) || ( c == '%' ) || ( c == '?' ) ||
    (c == '#') || (c == '(') || (c == '&')) {
	for( tklen = 0, numberF = TRUE;
	        ( c != EOF ); ++tklen ) {
		if (tklen == buf_siz - 1) {
			token_diff = tokenP - *token_start;
			*token_start = expand_buffer();
			buf_siz = buffer_size();
			tokenP = *token_start + token_diff;
		}
	    /* Check to see if char belongs in token, and at the
	       same time if it belongs in a number. */
	    if ( !isdigit(c) ) {
		if ( c == '-' ) {
		    if ( tklen != 0 )
			numberF = FALSE;
		}
		else if ( !isalpha(c) && (c != '#') && (c != '(') &&
	                  ( c != '!' ) && ( c != '?' ) && ( c != '%' ) &&
	 		  ( c != '.' ) && ( c != '$' ) && ( c != '\'' ) &&
			 (c != '/') && (c != '&'))
		    break;
		else
		    numberF = FALSE;
	    }

	    *tokenP++ = c;
	    c = getc( fP );
	}
	*tokenP = NUL;
	/* If it's a number, return that */
	if ( numberF ) {
	    ungetc( c, fP );
	    return( TKINTEGER );
	}

	if ( bolF ) {			/* Check for label */
	    if ( c == '=' ) 		/* Assignment */
		return( TKEQUATE );
	    if ( c == ':' ) {		/* Double-colon is global label */
		if ( ( c = getc( fP ) ) == ':' )
		    return( TKGLABEL );
		if ( c != EOF )
		    ungetc( c, fP );
		return( TKLABEL );
	    }
	}

	ungetc( c, fP );
	return( TKSYMBOL );
    }

    if ( c == '"' ) {			/* Some string? */
	for( tklen = 0; ( c = getc( fP ) ) != EOF; ++tklen ) {
	    if ( c == '"' ) {
		/* Check for doubled quote */
		if ( ( c = getc( fP ) ) != '"' ) {
		    ungetc( c, fP );	/* Reuse char */
		    break;		/* End of string */
		}
	    }
	    if ( tklen >= buf_siz - 1 ) {
			token_diff = tokenP - *token_start;
			*token_start = expand_buffer();
			buf_siz = buffer_size();
			tokenP = *token_start + token_diff;
		}
		*tokenP++ = c;
	}
        *tokenP = NUL;
        return( TKSTRING );
    }

    /* Something else. */
    *tokenP++ = c;
    *tokenP = NUL;
    return( TKCHAR );
}
/*
 
*//* lexadd( lxPP, lxXP, tktype, tokenP, tokenL )

	Add lexical entry to current stream

Accepts :

	lxPP		Address of ptr to LEXBUF in use
	lxXP		Address of lex buf index variable
	tktype		Type of token to add
	tokenP		The token to add
	tokenL		Number of bytes of token to add

Returns :

	<value>		Status code.
	*lxPP, *lxXP	Updated as necessary

*/

int
lexadd( lxPP, lxXP, tktype, tokenP, tokenL )
	LEXBUF		**lxPP;		/* Addr of lexbuf ptr */
	int		*lxXP;		/* Addr of lexbuf index var */
	int		tktype;		/* New token type */
	char		*tokenP;	/* New token */
	int		tokenL;		/* Length of token */
{
AREG1	LEXBUF		*lxP;
DREG1	unsigned int	lxX;

    lxP = *lxPP;
    lxX = *lxXP;

    /* Check for room in current buffer.  Need room for token type,
       token itself, and a block terminator. */
    if ( ( lxX + tokenL + 2 ) > (unsigned)LXBSIZE ) {

	/* Must allocate a continuation block */
	lxP->lx_data[lxX] = TKEOB;	/* Mark end of block */

	if ( ( lxP = (LEXBUF *)MALLOC( sizeof(LEXBUF) ) ) == NULL ) {
	    my_fprintf( stderr, "ZAP: Can't allocate LEXBUF(1) in lexadd()\n" );
	    return( SCMEMORY );
	}

	S_lexttl += sizeof(LEXBUF);
	S_lexsize += sizeof(LEXBUF);
	if ( S_lexsize > S_lexmax )
	    S_lexmax = S_lexsize;

	(*lxPP)->lx_link = lxP;
	*lxPP = lxP;
	lxX = 0;
    }

    /* Add new token */
    LasttkP = &lxP->lx_data[lxX++];
    *LasttkP = tktype;
    if ( tokenL != 0 ) {
	memcpy( LasttkP+1, tokenP, tokenL );
	lxX += tokenL;
    }

    *lxXP = lxX;
    return( SCOK );
}
/*
 
*//* litstr( strP )

	Return pointer to literal string pointer

Accepts :

	strP		String to reference

Returns :

	<value>		Pointer to ptr to a single copy of the string.

*/
 
char **
litstr( strP )
	char		*strP;
{
	SYMBOL		*symP;		/* Symbol table ptr */

    /* Make or reference an entry in the literal table */
    symP = symenter( Litsymtab, strP, sizeof( SYMBOL ) );
    return( &(symP->sym_name) );
}


/***
	buffer allocation
	
	This module allocates buffers to be used for tokens.  It is set up
to handle recursion and possible changing buffer sizes.

***/

static int num_buffers = 0; /* Number of buffers */
static int cur_buffer = -1;  /* Current buffer */
static char **buffers = NULL;		/* Array of buffers */
static int *buf_sizes = NULL;

char
*get_buffer()
{
	
	if (++cur_buffer > num_buffers - 1) {
		num_buffers = num_buffers + 100;
		buffers = (char **)REALLOC(buffers,num_buffers * sizeof(char **));
		buf_sizes = (int *)REALLOC(buf_sizes,num_buffers * sizeof(int *));
	}
	if (buf_sizes[cur_buffer] == 0) {
		buf_sizes[cur_buffer] = STRINGBUF;
		buffers[cur_buffer] = MALLOC(STRINGBUF);
	}
	return(buffers[cur_buffer]);
}


free_buffer()
{
	cur_buffer--;
}


char
*expand_buffer()
{
	buf_sizes[cur_buffer] += STRINGBUF;
	buffers[cur_buffer] = REALLOC(buffers[cur_buffer],buf_sizes[cur_buffer]);
	if (buf_sizes[cur_buffer] > Max_buf_len)
		Max_buf_len = buf_sizes[cur_buffer];
	return(buffers[cur_buffer]);
}

int
buffer_size()
{
	return(buf_sizes[cur_buffer]);
}



