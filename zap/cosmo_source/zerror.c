/*	zerror.c	Error handling module

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	zerror		Output error message for current line
	zexpected	Output "expected something" error message

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern	FILE	*fopen();

/* External data */

extern	char	Curfnm[];
extern	UWORD	Curline;
extern	long	Curseek;
extern	FILE	*ErrfP;
extern	FILE	*Error_out;
extern	char	Errfnm[];
extern	UWORD	Errline;
extern	BOOL	Opsrcerr;
extern	int	Pass;

/* Local routines (and forward references) */


/* Local data */


/* Private data */

my_printf(msgP, s1, s2, s3, s4)
char *msgP, *s1, *s2, *s3, *s4;
{
  printf(msgP, s1, s2, s3, s4);
  if (Error_out)
    fprintf(Error_out, msgP, s1, s2, s3, s4); }

my_fprintf(ff, msgP, s1, s2, s3, s4)
char *msgP, *s1, *s2, *s3, *s4;
FILE *ff;
{
  fprintf(ff, msgP, s1, s2, s3, s4);
  if (Error_out)
    fprintf(Error_out, msgP, s1, s2, s3, s4); }

/*
 
*//* zerror( when, msgP, s1, s2, s3, s4 )

	Output an error message

Accepts :

	when		Mask telling when error should be displayed
	msgP		Address of error message string
	s1-s4		Any char * arguments to printf the error message.

Returns :


*/

int
zerror( when, msgP, s1, s2, s3, s4 )
	int		when;		/* Mask of appropriateness */
	char		*msgP;		/* Message string */
	char		*s1, *s2, *s3, *s4;
{
	int		c;
	int		i;
	char		sourceline[STRINGBUF+1];

    /* If no appropriate "when" bits set, don't process. */
    if ( ( ( Pass == 0 ) && ( ( when & E_PASS1 ) == 0 ) ) ||
         ( ( Pass != 0 ) && ( ( when & E_PASS2 ) == 0 ) ) )
	return;

    if ( Opsrcerr ) {			/* If printing source line */
	/* Position source line printing */
	if ( strcmp( &Curfnm[0], &Errfnm[0] ) != 0 ) {
	    strcpy( &Errfnm[0], &Curfnm[0] );
	    if ( ErrfP != NULL ) {
		fclose( ErrfP );
		ErrfP = NULL;
	    }
	}
	
	/* Make sure source file is open */
	if ( ErrfP == NULL ) {
	    if ( ( ErrfP = fopen( &Errfnm[0], "r" ) ) == NULL ) {
	      my_fprintf(stderr, "ZAP: error opening \"%s\" (zerror)\n",
			 &Errfnm[0] );
	      return;
	    }
	    Errline = -1;
	}

	/* Obtain the source line for printing */
	if ( Errline != Curline ) {
	    fseek( ErrfP, Curseek, 0 );
	    Errline = Curline;

	    for( i = 0; ( c = getc( ErrfP ) ) != EOF; ) {
		if ( c == '\n' )
		    break;
		if ( i < STRINGBUF )
		    sourceline[i++] = c;
	    }
	    sourceline[i] = NUL;

	    /* Print source line information */
	    my_fprintf( stderr, "\"%s\", line %d: %s\n", 
		  &Errfnm[0], Errline, &sourceline[0] );
	}

	my_fprintf( stderr, "   -- " );	/* Lead in to the error */
    }
    else {				/* Not printing source line */
      my_fprintf( stderr, "\"%s\", line %d: ",
		  &Curfnm[0], Curline ); }

    /* Print the error stuff */
    my_fprintf( stderr, msgP, s1, s2, s3, s4 );
    my_fprintf( stderr, "\n" );
}

my_exit(code)
int code;
{
  if (Error_out)
    fclose(Error_out);
  exit(code); }

/*
 
*//* zexpected( when, what )

	Output an error message about expecting what, and flush the line.

Accepts :

	when		Mask specifying when message is appropriate
	what		Ptr to description string

Returns :

	< sts >		Status from assync()


Notes :

	The line is flushed whether or not the message is displayed.

*/

int
zexpected( when, what )
	int		when;		/* Message appropriateness */
	char		*what;		/* Description */
{
    zerror( when, "syntax error -- expected %s", what );
    return( assync( FALSE ) );
}
