/*	zlabel.c	Process a label definition

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	asequate	Process an equate
	aslabel		Process a label

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern	long	objtell();
extern	SYMBOL 	*symenter();
extern	SYMBOL	*symlookup();

/* External data */

extern	STABLE	*Gblsymtab;
extern	STABLE	*Lclsymtab;
extern	UBYTE	*LextkP;		/* Lex token pointer */
extern	int	Pass;
extern	UWORD	State;

extern	UBYTE	cur_type; /* Used for symbol table generation. */
extern 	LGSYMBOL *cur_label;
extern unsigned char num_labels;
extern LGSYMBOL **label_list;
extern LGSYMBOL *cur_gstring;

/* Local routines (and forward references) */


/* Local data */


/* Private data */

/*
 
*//* asequate()

	Process an equate

Accepts :


Returns :

	<value>		Status code.

*/

int asequate()
{
  DREG1	int		sts;
  VALUE		val;		/* Symbol value */
  char		*nameP;		/* Ptr to symbol name */
  AREG1	LGSYMBOL	*symP;		/* Ptr to symbol table entry */
  int got_type;

  /* get ptr to equate symbol name */
  memcpy( &nameP, LextkP+1, sizeof(char *) );
  if (strcmp(nameP, "LAST-OBJECT") == 0)
    nameP = nameP;

  if ( ( sts = asnxtoken() ) != SCOK )
    return( sts );

  /* Get the equate value */
  if ( ( sts = eval( &val, NULL ) ) != SCOK )
    return( sts );

  /* Make sure value is absolute */
  if ( ( val.v_flags & ST_REL ) != 0 )
    /* Just give warning, and proceed */
    zerror( E_PASS1, "warning - non-absolute value in equate" );
  
  /* Enter symbol/value in symbol table */
  symP = (LGSYMBOL *)symenter( Gblsymtab, nameP, sizeof( LGSYMBOL ) );

  /* Make the entry */
  symP->lg_val.v_flags = ST_GLOBAL|ST_DEFINED;
  symP->lg_val.v_value = val.v_value;
  symP->lg_loc = objtell();		/* Where defined */
  if ((got_type = get_type(TRUE)) == ANY_SPACE)
    got_type = cur_type;	/* If no space specified in equate, use expr type */
  symP->lg_space = got_type;
	
  /* Check if we should half-kill symbol */
  if ((symP->lg_space == ZSTRING_SPACE) && (cur_gstring != NULL) &&
      (cur_gstring->lg_val.v_value = symP->lg_val.v_value))
    cur_gstring->lg_val.v_flags |= ST_KILL;

  assync( TRUE );			/* Catch any random stuff */
  return( assync( FALSE ) );
}
/*
 
*//* aslabel()

	Called when the current token is a label

Accepts :


Returns :

	<value>		Status code.

*/

int
aslabel()
{
DREG1	int		sts;
	int		tktype;		/* Type of label */
	int		gblF;		/* Whether global */
	char		*nameP;		/* Ptr to symbol name */
AREG1	LGSYMBOL	*symP;		/* Ptr to symbol table entry */

    tktype = *LextkP;			/* Get label token type */
    memcpy( &nameP, LextkP+1, sizeof(char *) ); /* Ptr to symbol name */

    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Decide on global vs local */
    if ( tktype == TKGLABEL )
	gblF = TRUE;
    else {
	/* Local label syntax - but still might be global */
	if ( ( State & AS_FUNCT ) == 0 )
	    /* Not in a function */
	    gblF = TRUE;
	else
	    gblF = FALSE;
    }

    /* Attempt to find or insert the symbol */
    if ( gblF )
	symP = (LGSYMBOL *)symenter( Gblsymtab, nameP, sizeof( LGSYMBOL ) );
    else
	symP = (LGSYMBOL *)symenter( Lclsymtab, nameP, sizeof( LGSYMBOL ) );

    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) ) {
	/* Duplicate definition */
	zerror( E_PASS1, "symbol \"%s\" is multiply defined", nameP );
	return( assync( FALSE ) );
    }

    /* Setup the value with the current PC */
    symP->lg_val.v_value = (ZNUM)( symP->lg_loc = objtell() );
    symP->lg_val.v_flags = ST_REL|ST_DEFINED;
    symP->lg_space = LABEL_SPACE;
    if ( gblF )
	symP->lg_val.v_flags |= ST_GLOBAL;
	
	/* Check the previous label to see if it had the same value.  If so,
	   half-kill it.  Set this label as the most recent. */
	if (cur_label != NULL) 
		if ((cur_label->lg_val.v_value == symP->lg_val.v_value) &&
		    ((State & AS_VOCAB) == 0) &&
		    (gblF))
			cur_label->lg_val.v_flags |= ST_KILL;
		else
			num_labels = 0;
	else
		num_labels = 0;
        if (num_labels == MAX_LABELS)
	  num_labels--;
	cur_label = label_list[num_labels++] = symP;
	
    /* If we're in a vocabulary section, which will be sorted at the end
       of the section and added to the object data, make a call to
       associate the label with the vocabulary data. */
    if ( ( State & AS_VOCAB ) != 0 )
	sts = voclabel( symP );

    return( sts );
}
