/*	zeval.c		Expression evaluator

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	evalbyte	Evaluate, output a byte of object data
	evalconst	Evaluate an expression which must be a constant
	evalword	Evaluate, output a word of object data
	eval		Evaluate the next expression in the input stream.
	evalsymref	Find a symbol intelligently

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
extern	UBYTE	*LextkP;
extern	int	Pass;
extern	UWORD	State;
extern LGSYMBOL *cur_funct;
extern UBYTE cur_type; /* Used for symbol table generation */
extern int VocX;

/* Local routines (and forward references) */

	LGSYMBOL	*evalsymref();

/* Local data */


/* Private data */

/*
 
*//* evalbyte()

	Collect an argument value and output a byte of object data with
	  potential fixup information.

Accepts :


Returns :

	<value>		status code

*/

int evalbyte()
{
  DREG1	int		sts;
  long		curaddr;
  VALUE		val;		/* Value from expression */
  LGSYMBOL	*symP;		/* Ptr to any symbol made */

  if ( ( sts = eval( &val, &symP ) ) != SCOK )
    return( assync( FALSE ) );	/* If error, flush line */
  
  /* If absolute value, go ahead and process it */
  if ( ( val.v_flags & ST_REL ) == 0 ) {
    if ( val.v_value > 0xff )	/* Check byte range */
      zerror( E_ALWAYS, "warning - value truncated to LSB" );
    return( objputb( (UWORD)val.v_value ) );
  }

  /* Value is a label reference. */
  
  if ((State & AS_VOCAB) != 0)
    curaddr = VocX;
  else
    curaddr = objtell();

  if ( ( sts = objputb( (UWORD)val.v_value ) ) != SCOK )
    return( sts );

  /* If it's a forward reference, enter fixup information */
  if ( ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) ||
      ( symP->lg_loc > curaddr ) )
    sts = makefixup( symP, FXBYTE, curaddr, 0 );

  return( sts );
}
/*
 
*//* evalconst( resultP )

	Get the next constant value.

Accepts :

	resultP		Ptr to where to put the value

Returns :

	<value>		Status code
	*resultP	If success, the constant value.

*/

int
evalconst( resultP )
	ZNUM		*resultP;	/* Where to put the answer */
{
	VALUE		val;		/* Value from expression */

    if ( eval( &val, NULL ) != SCOK )
	return( assync( FALSE ) );	/* Return if problem */

    if ( ( val.v_flags & ST_REL ) != 0 ) {
	zerror( E_ALWAYS, "warning - invalid use of relocatable value" );
	*resultP = 0;
    }
    else
	*resultP = val.v_value;

    return( SCOK );
}
/*
 
*//* evalword()

	Get the next value and output a word of object data with
	  potential fixup information.

Accepts :


Returns :

	<value>		status code

*/

int evalword()
{
  DREG1	int		sts;
  long		curaddr;
  VALUE		val;		/* Value from expression */
  LGSYMBOL	*symP;		/* Ptr to any symbol made */

  if ( ( sts = eval( &val, &symP ) ) != SCOK )
    return( assync( FALSE ) );	/* Return if bad */

  if ((State & AS_VOCAB) != 0)
    curaddr = VocX;
  else
    curaddr = objtell();

  /* Output the value */
  if ( ( sts = objputw( (WORD)val.v_value ) ) != SCOK )
    return( sts );

  /* If it's a forward reference, enter fixup information */
  if ( ( ( val.v_flags & ST_REL ) != 0 ) &&
      ( ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) ||
       ( symP->lg_loc > curaddr ) ) )
    sts = makefixup( symP, FXWORD, curaddr, 0 );

  return( sts );
}
/*
 
*//* eval( valP, symPP )

	Evaluate a value expression

Accepts :

	valP		Where to store the value block
	symPP		Where to store ptr to symbol table entry

Returns :

	<value>		status code
	*valP		The value data; pertinent flag bit is ST_REL.
	*symPP		If relocatable value, the address of the symbol
			  table entry for the symbol that is the value.

Notes :

	A relocatable value can consist only of a reference to a label
with no arithmetic operators or other terms.  A constant expression can
consist of any number of *defined* terms connected by plus signs.

*/

int
eval( valP, symPP )
AREG1	VALUE		*valP;		/* Where to store value */
	LGSYMBOL	**symPP;	/* Ptr to symbol table entry var */
{
DREG2	int		sts;
	ZNUM		sv;		/* Scratch value */
DREG1	ZNUM		value;		/* Value we're making */
        char		*strP;		/* Ptr to a string */
	LGSYMBOL	*symP;		/* Symbol table entry */
	LGSYMBOL	*dummysymP;
        BOOL            sym_used = FALSE;

    if ( symPP == NULL )
	symPP = &dummysymP;

    *symPP = NULL;			/* No symbol address */
	
	cur_type = CONSTANT_SPACE;

    for( value = 0 ; ; ) {
	switch( *LextkP ) {		/* Dispatch by type of token */
	    case TKSYMBOL:
		memcpy( &strP, LextkP+1, sizeof(char *) );
		symP = evalsymref( strP );	/* Find symbol */
		if ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) {
		    symP->lg_val.v_value = 0;   /* Make sure value is 0 */
		    symP->lg_val.v_flags |= ST_REL;
		}
		/* Deal with label symbol */
		if ( ( symP->lg_val.v_flags & (ST_REL|ST_VAR) ) != 0 ) {
		    if ( sym_used ) {
			/* Label/var reference must be only one */
			zerror( E_PASS1, "invalid expression" );
			return( SCNOTDONE );
		    }
		    sym_used = TRUE;
		    *symPP = symP;	/* Remember reference ptr */
		}

		/* Add in this value */
		value += symP->lg_val.v_value;
		
		/* Set current type */
		if (cur_type == CONSTANT_SPACE)
			cur_type = symP->lg_space;
		else if (cur_type != symP->lg_space)
			cur_type = ANY_SPACE;
		
		break;

	    case TKINTEGER:
		memcpy( &sv, LextkP+1, sizeof(ZNUM) );
		value += sv;
		cur_type = ANY_SPACE;
		break;

	    default:
		zerror( E_PASS1, "invalid expression" );
		return( SCNOTDONE );
	}

	/* Check for plus between terms */
	if ( ( sts = asnxtoken() ) != SCOK )
	    return( sts );

	if ( ( *LextkP == TKCHAR ) && ( LextkP[1] == '+' ) ) {
       	    if ( ( sts = asnxtoken() ) != SCOK )
		return( sts );
	}
	else
	    break;

    }

    /* Fill in the answer */
    valP->v_flags = (*symPP == NULL) ? 0 : (*symPP)->lg_val.v_flags;
    valP->v_value = value;
	
	if (cur_type == ANY_SPACE)
		cur_type = CONSTANT_SPACE;

    return( SCOK );
}
/*
 
*//* evalsymref( nameP )

	Perform a reference to a symbol

Accepts :

	nameP		Name of symbol to find

Returns :

	<value>		Ptr to symbol table entry

Notes :

	This routine attempts to find a symbol the right way, using
current assembler context to determine which tables to search and
to enter the symbol in if it does not exist.
Note: We assume that any undefined symbol at this point is GLOBAL. Local labels will
not be processed by this function.
It also passes on info for segment file generation, if the argument is either a
function name or an undefined symbol.  An undefined symbol may be a function name
or a string; if its not either, it will be thrown out later.  This information is
needed because functions or strings referenced in a function must be included in that
function's segment.

*/

LGSYMBOL *evalsymref( nameP )
AREG2	char		*nameP;		/* name to find */
{
  AREG1	LGSYMBOL	*symP;		/* Ptr to symbol */

  /* Process by whether in a function */
  if ( ( State & AS_FUNCT ) != 0 ) {
    /* Try local symbol table first */
    symP = (LGSYMBOL *)symlookup( Lclsymtab, nameP );
    if ( ( symP != NULL ) &&
	( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
      return( symP );		/* Found valid reference */
  }

  /* Not in function, or not defined within the function --  global reference. */
  symP = (LGSYMBOL *)symenter( Gblsymtab, nameP, sizeof(LGSYMBOL) );
  symP->lg_val.v_flags |= ST_GLOBAL;
  symP->lg_referenced = 1;
  if (((symP->lg_val.v_flags & ST_DEFINED) == 0) ||
      ((symP->lg_space == FUNCTION_SPACE)))
    addcall(cur_funct,symP);
  return( symP );
}
