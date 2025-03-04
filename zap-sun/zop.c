/*	zop.c		Opcode processing

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	zo_0op		Process 0-op opcodes
	zo_1op		Process 1-op opcodes
	zo_2op		Process 2-op opcodes
	zo_xop		Process x-op opcodes
	zo_jump		Process jump opcode(s)
	zo_printi	Process opcode(s) with immediate string arg
	zo_call		Special for CALL
	zo_icall	Special for ICALL

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */

#define __SEG__ Seg2

	/* opcode argument types */
#define	AT_VAR		1		/* Variable */
#define	AT_CONSTS	2		/* Short Constant */
#define	AT_CONSTL	3		/* Long Constant */
#define	AT_OTHER	4		/* Other */

/* External routines */

extern	long	objtell();
extern	SYMBOL 	*symenter();
extern	SYMBOL	*symlookup();
extern	UBYTE	*zstr();

/* External data */

extern	int	build_freq;
extern	STABLE	*Gblsymtab;
extern	STABLE	*Lclsymtab;
extern	UBYTE	*LextkP;
extern	int	Pass;
extern	UWORD	State;

/* Local routines (and forward references) */


/* Local data */

static	int	ArgC;			/* Number of opcode arguments */
static	struct oparg {			/* Argument information */
	 int	arg_type;		/* Assumed type */
	 VALUE	arg_val;		/* Argument value */
	 LGSYMBOL *arg_sym;		/* Symbol (if any) */
	 }	Argtbl[8];

/* Private data */

/*
 
*//* zo_0op( ocP )

	Process a 0-op opcode


Accepts :

	ocP		Pointer to opcode info

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_0op( ocP )
	ODDISP		*ocP;		/* Ptr to opcode data */
{
    objputb( ocP->od_value );		/* Output the opcode value */
    return( zo_eol( ocP ) );		/* Process end of line */
}
/*
 
*//* zo_1op( ocP )

	Process a 1-op opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_1op( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
DREG2	UBYTE		opcode;		/* Opcode value */

    if ( ( sts = zo_getargs( 1 ) ) != SCOK )
	return( sts );

    if ( ArgC != 1 ) {			/* Must be one arg */
	zerror( E_PASS1, "too few arguments." );
	return( assync( FALSE ) );
    }

    opcode = ocP->od_value;
    if ( Argtbl[0].arg_type == AT_VAR )
	opcode |= 0x20;			/*  set variable */
    else if ( Argtbl[0].arg_type == AT_CONSTS )
	opcode |= 0x10;			/*  short immediate */

    if ( ( sts = objputb( opcode ) ) == SCOK )
	if ( ( sts = zo_putargs() ) == SCOK )
	    sts = zo_eol( ocP );

    return( sts );
}
/*
 
*//* zo_2op( ocP )

	Process a 2-op opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_2op( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
	int		maxargs;
	UWORD		opcode;		/* The opcode value (note that
					   its value may be bigger than
					   a byte value) */
	int		t1, t2;		/* Types for arg1 and arg2 */

    /* Collect arguments */
    if ( ( ocP->od_flags & ST_VARARGS ) == 0 )
	maxargs = 2;
    else
	maxargs = 8;
    if ( ( sts = zo_getargs( maxargs ) ) != SCOK )
	return( sts );

    if ( ArgC < 2 ) {			/* Must be at least two args */
	zerror( E_PASS1, "too few arguments." );
	return( assync( FALSE ) );
    }

    opcode = ocP->od_value;

    /* See if we have to do x-op format */
    t1 = Argtbl[0].arg_type;
    t2 = Argtbl[1].arg_type;
    if ( ( ArgC > 2 ) ||
	 ( ( t1 != AT_VAR ) && ( t1 != AT_CONSTS ) ) ||
         ( ( t2 != AT_VAR ) && ( t2 != AT_CONSTS ) ) ) {
	/* Must use extended op format */
	opcode += BIAS2OP;		/* Shift into xop range */
	if ( opcode > 0xff ) {
	    /* Must also use prefix byte */
	    if ( ( sts = objputb( EXTOP ) ) == SCOK )
		sts = objputb( opcode - 256 );
	}
	else
	    sts = objputb( opcode );

	if ( sts == SCOK )
	    /* Output args in xop format */
	    if ( ( sts = zo_putxargs() ) == SCOK )
		return( zo_eol( ocP ) );
    }

    /* Finish opcode and output the results */
    if ( t1 == AT_VAR )
	opcode |= 0x40;
    if ( t2 == AT_VAR )
	opcode |= 0x20;

    if ( ( sts = objputb( opcode ) ) == SCOK )
	if ( ( sts = zo_putargs() ) == SCOK )
	    sts = zo_eol( ocP );

    return( sts );
}
/*
 
*//* zo_xop( ocP )

	Process a x-op opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_xop( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
    return( zo_gxop( ocP, 0 ) );	/* Do general xop processing */
}
/*
 
*//* zo_call( ocP )

	Process a CALL opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_call( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
    return( zo_gxop( ocP, 12 ) );	/* Do general xop processing */
}
/*
 
*//* zo_icall( ocP )

	Process ICALL opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_icall( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
    return( zo_gxop( ocP, 1 ) );	/* Do general xop processing */
}
/*
 
*//* zo_gxop( ocP, mdelta )

	General routine to process x-ops

Accepts :

	ocP		Pointer to the opcode data
	mdelta		How much to add to opcode if > 4 arguments

Returns :

	<value>		Status code: SCOK to continue, other to abort.
*/

int
zo_gxop( ocP, mdelta )
	ODDISP		*ocP;		/* Opcode data ptr */
	int		mdelta;		/* Delta for many arguments */
{
DREG1	int		sts;
	UWORD		opcode;		/* Opcode value (may be bigger
					   than a byte value) */

    /* Collect arguments */
    if ( ( sts = zo_getargs( 8 ) ) != SCOK )
	return( sts );

    /* Output the opcode and the arguments */
    opcode = ocP->od_value;
    if ( ArgC > 4 )			/* If many args */
	opcode += mdelta;		/*  shift the opcode value */

    if ( opcode > 0xff ) {		/* Needs prefix byte */
	if ( ( sts = objputb( EXTOP ) ) == SCOK )
	    sts = objputb( opcode - 256 );
    }
    else
	sts = objputb( opcode );

    /* Output the arguments in xop format */
    if ( sts == SCOK )
	if ( ( sts = zo_putxargs() ) == SCOK )
	    sts = zo_eol( ocP );

    return( sts );
}
/*
 
*//* zo_jump( ocP )

	Process a jump opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_jump( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
	long		curaddr;
	WORD		offset;		/* Jump displacement */
	char		*snameP;	/* Ptr to symbol name */
AREG1	LGSYMBOL	*symP;		/* Ptr to symtab entry */

    /* Argument must be symbolic branch location */
    if ( !anyarg() || ( *LextkP != TKSYMBOL ) ) {
	zerror( E_PASS1, "expected branch location" );
	return( SCNOTDONE );
    }

    memcpy( &snameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Lookup symbol in local symbol table */
    symP = (LGSYMBOL *)symenter( Lclsymtab, snameP, sizeof( LGSYMBOL) );

    /* Output the opcode and relative branch value */
    if ( ( sts = objputb( ocP->od_value ) ) != SCOK )
	return( sts );

    curaddr = objtell();
    offset = (WORD)(symP->lg_loc - curaddr);

    if ( ( sts = objputw( offset ) ) != SCOK )
	return( sts );

    /* Check for forward or undefined reference. */
    if ( ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) ||
         ( symP->lg_loc > curaddr ) ) {
	/* Issue a fixup for this */
	symP->lg_val.v_flags |= ST_REL;
	if ( ( sts = makefixup( symP, FXJUMP, curaddr, 0 ) ) != SCOK )
	    return( sts );
    }

    return( zo_eol( ocP ) );
}
/*
 
*//* zo_printi( ocP )

	Process a printi opcode


Accepts :

	ocP		Pointer to the opcode data

Returns :

	<value>		Status code: SCOK to continue, other to abort.

*/

int
zo_printi( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
AREG1	UBYTE		*strP;		/* Ptr to the string */

    /* Get the string argument */
    if ( *LextkP != TKSTRING ) {
	zerror( E_PASS1, "expected string." );
	return( SCNOTDONE );
    }

    strP = LextkP+1;
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Output opcode and z-string, and do end-of-line stuff */
    if ( ( sts = objputb( ocP->od_value ) ) == SCOK ) {
	strP = zstr( strP, TRUE, 0 );	/* Translate arg to Z-string */
	if ( ( sts = objputzstr( strP ) ) == SCOK )
	   sts = zo_eol( ocP );
    }
    return( sts );
}
/*
 
*//* zo_eol( ocP )

	Do end-of-line processing

Accepts :

	ocP		Ptr to opcode data

Returns :

	<value>		Status code

*/

static int
zo_eol( ocP )
	ODDISP		*ocP;		/* Ptr to opcode data */
{
DREG1	int		sts;

    /* Process output value if requred */
    if ( ( ocP->od_flags & ST_VAL ) != 0 )
	if ( ( sts = zo_val( ocP ) ) != SCOK )
	    return( sts );

    /* Process predicate branch if required */
    if ( ( ocP->od_flags & ST_PRED ) != 0 )
	if ( ( sts = zo_pred( ocP ) ) != SCOK )
	    return( sts );

    /* Do syntax check for end-of-line */
    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zo_val( ocP )

	Process output value for opcode

Accepts :

	ocP		Ptr to opcode's data block

Returns :

	<value>		status code

*/

static int
zo_val( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
	char		*vnameP;	/* Ptr to variable name */
AREG1	LGSYMBOL	*symP;		/* Ptr to symtab entry */

    /* Look for ">" signalling value */
    if ( ( *LextkP != TKCHAR ) || ( LextkP[1] != '>' ) )
	/* If none, assume default of zero (stack) */
	return( objputb( 0 ) );

    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Next thing must be variable name */
    if ( *LextkP != TKSYMBOL ) {
	zerror( E_PASS1, "expected variable name" );
	return( SCNOTDONE );
    }

    memcpy( &vnameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Lookup symbol in local symbol table, if in function */
    if ( ( State & AS_FUNCT ) != 0 )
	symP = (LGSYMBOL *)symlookup( Lclsymtab, vnameP );

    /* Try global symbol table if necessary */
    if ( ( ( State & AS_FUNCT ) == 0 ) || ( symP == NULL ) )
	symP = (LGSYMBOL *)symlookup( Gblsymtab, vnameP );

    if ( symP == NULL ) {
	zerror( E_PASS1, "undefined variable \"%s\".", vnameP );
	return( SCNOTDONE );
    }

    if ( ( symP->lg_val.v_flags & ST_VAR ) == 0 ) {
	zerror( E_PASS1, "\"%s\" is not a variable.", vnameP );
	return( SCNOTDONE );
    }

    /* Output the variable number */
    return( objputb( symP->lg_val.v_value ) );
}
/*
 
*//* zo_pred( ocP )

	Process predicate argument (branch) for opcode

Accepts :

	ocP		Ptr to opcode's data block

Returns :

	<value>		status code

*/

static int
zo_pred( ocP )
	ODDISP		*ocP;		/* Opcode data ptr */
{
DREG1	int		sts;
	UBYTE		sense;		/* Sense of branch */
	long		curaddr;
	UWORD		bvalue;		/* Branch value */
	char		*vnameP;	/* Ptr to variable name */
AREG1	LGSYMBOL	*symP;		/* Ptr to symtab entry */

    /* Look for "/" or "\" for predicate branch */
    if ( ( *LextkP == TKCHAR ) && ( LextkP[1] == '\\' ) )
	sense = 0x00;			/* branch on false */
    else if ( ( *LextkP == TKCHAR ) && ( LextkP[1] == '/' ) )
	sense = 0x80;			/* Branch on true */
    else {
	zerror( E_PASS1, "expected \"\\\" or \"/\"" );
	return( objputb( 0x40 ) );	/* Default? */
/*	return( SCNOTDONE );  */
    }

    /* Skip the slash */
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Next thing must be variable name */
    if ( *LextkP != TKSYMBOL ) {
	zerror( E_PASS1, "expected variable name" );
	return( SCNOTDONE );
    }
    memcpy( &vnameP, LextkP+1, sizeof(char *) );

    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Check for special "TRUE" or "FALSE" */
    if ( strcmp( vnameP, "FALSE" ) == 0 )
	return( objputb( sense | 0x40 ) ); /* Output false thing */
    if ( strcmp( vnameP, "TRUE" ) == 0 )
	return( objputb( sense | 0x41 ) ); /* TRUE indicator */

    /* Lookup symbol in local symbol table only. */
    symP = (LGSYMBOL *)symenter( Lclsymtab, vnameP, sizeof( LGSYMBOL) );

    curaddr = objtell();

    /* Check for forward references. */
    if ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) {
        /* Undefined symbol; use full 14-bit fixup */
	symP->lg_val.v_flags |= ST_REL;
	if ( ( sts = objputw( sense<<8 ) ) == SCOK )
	    sts = makefixup( symP, sense == 0x80 ? FXTPREDL : FXFPREDL,
					 curaddr, 0 );
	return( sts );
    }

    /* Check for forward reference on "second" pass.  Note that we'll
       attempt to output a reasonable value even though this value will
       be overwritten later during the fixup process. */
    if ( symP->lg_loc > curaddr ) {
	bvalue = symP->lg_loc - (curaddr +1) +2;
	if ( ( bvalue & ~0x3f ) == 0 ) {
	    /* Single byte forward reference */
	    if ( ( sts = objputb( sense | bvalue | 0x40 ) ) == SCOK )
		sts = makefixup( symP, sense == 0x80 ? FXTPREDS : FXFPREDS,
					 curaddr, 0 );
	}
	else {
	    /* Two-byte forward reference */
	    --bvalue;			/* Offset decreased 'cause of extra
					    byte in offset value. */
	    if ( ( sts = objputw( ( sense<<8 ) | bvalue ) ) == SCOK )
		sts = makefixup( symP, sense == 0x80 ? FXTPREDL : FXFPREDL,
					 curaddr, 0 );
	}
	return( sts );
    }

    /* Backward reference */
    bvalue = symP->lg_loc - (curaddr+2) +2;
    if (( ( bvalue & 0xc000 ) != 0xc000 ) && !build_freq) {
        /* Can't fit in 14 bits */
	zerror( E_ALWAYS, "predicate reference too far" );
	sts = SCNOTDONE;
    }
    else {
	bvalue = ( sense << 8 ) | ( bvalue & 0x3fff );
	sts = objputw( bvalue );
    }

    return( sts );
}
/*
 
*//* zo_putxargs()

	Output args for x-op format

Accepts:

Returns :

	<value>		status code

*/

static int
zo_putxargs()
{
DREG1	int		sts;
	BYTE		argN;		/* Arg number */
	BYTE		maskN;		/* Arg-in-mask */
	UBYTE		argmask;
AREG1	struct oparg	*argP;

    /* Go through the args && make the masks */
    for( argN = maskN = argmask = 0, argP = &Argtbl[0];
			 argN < ArgC;
			 ++argN, ++maskN, ++argP ) {

	if ( maskN == 4 ) {		/* Ready to output maskbyte */
	    if ( ( sts = objputb( argmask ) ) != SCOK )
		return( sts );
	    maskN = argmask = 0;
	}

	argmask = argmask << 2;		/* Shift out for next thing */
	switch ( argP->arg_type ) {
	    case AT_VAR:		/* variable */
		argmask |= 2;
		break;

	    case AT_CONSTS:		/* Short immediate */
		argmask |= 1;
		break;

	    case AT_CONSTL:		/* Long immediate */
	    case AT_OTHER:
		break;
	}
    }

    /* Finish up this mask */
    while( maskN++ < 4 )
	argmask = ( argmask << 2 ) | 0x3;

    if ( ( sts = objputb( argmask ) ) != SCOK )
	return( sts );
	
    /* Now output the arg values */
    return( zo_putargs() );
}
/*
 
*//* zo_putargs()

	Output opcode arguments

Accepts :


Returns :

	<value>		Status code

*/

static int
zo_putargs()
{
DREG1	int		sts;
DREG2	int		i;
	long		curaddr;
AREG1	struct oparg	*argP;
	LGSYMBOL	*symP;

    curaddr = objtell();		/* Current object file addr */

    /* Go through and output the args */
    for( sts = SCOK, i = 0 , argP = &Argtbl[0];
		( sts == SCOK ) && ( i < ArgC );
		 ++i, ++argP ) {
	switch( argP->arg_type ) {	/* Process by arg type */
	    case AT_VAR:
	    case AT_CONSTS:
		sts = objputb( (UBYTE) argP->arg_val.v_value );
		++curaddr;
		break;

	    case AT_CONSTL:
		sts = objputw( (UWORD) argP->arg_val.v_value );
		curaddr += 2;
		break;

	    case AT_OTHER:
		symP = argP->arg_sym;	/* Get symbol ptr */
		if ( ( sts = objputw( (UWORD)argP->arg_val.v_value ) ) == SCOK ) {
		    /* Check for fixup required */
		    if ( ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 ) ||
		         ( symP->lg_loc > curaddr ) )
			sts = makefixup( symP,FXWORD,curaddr,argP->arg_val.v_value );
		    curaddr += 2;
		}
		break;

	}
    }
    return( sts );
}
/*
 
*//* zo_getargs( argmax )

	Get opcode arguments

Accepts :

	argmax		Maximum number allowed

Returns :

	<value>		Status code

*/

static int
zo_getargs( argmax )
	int		argmax;
{
DREG1	int		sts;
	UBYTE		quoted;
	UWORD		flags;
AREG1	struct oparg	*argP;


    for( ArgC = 0 , argP = &Argtbl[0]; ArgC < 9; ++ArgC, ++argP ) {
	/* Check for variable quoting */
	if ( ( *LextkP == TKCHAR ) && ( LextkP[1] == '\'' ) ) {
	    quoted = TRUE;
	    if ( ( sts = asnxtoken() ) != SCOK )
		return( sts );
	}
	else
	    quoted = FALSE;

	if ( !anyarg() )		/* If no more args on line */
	    break;

	/* Get next argument */
	sts = eval( &(argP->arg_val), &(argP->arg_sym) );
	if ( sts != SCOK )
	    return( sts );

	/* Guess argument type */
	flags = (argP->arg_val).v_flags;
	if ( ( flags & ST_VAR ) != 0 ) {
	    if ( quoted ) {
		argP->arg_type = AT_CONSTS;	/* Treat as immediate */
	    }
	    else
		argP->arg_type = AT_VAR;	/* Treat as variable */
	}
	else if ( quoted )
	    zerror( E_PASS1, "syntax error - misplaced quote." );
	else if ( ( flags & ST_REL ) == 0 ) {
	    if ( ( argP->arg_val.v_value & ~0xff ) == 0 )
		argP->arg_type = AT_CONSTS;  /* Short constant */
	    else
		argP->arg_type = AT_CONSTL;  /* Long constant */
	}
	else
	    argP->arg_type = AT_OTHER;

	ascomma();			/* Skip any separator */
    }

    if ( ArgC > argmax ) {
	zerror( E_PASS1, "too many arguments for opcode" );
	return( SCNOTDONE );
    }
    return( SCOK );
}
