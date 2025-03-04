/*	zdir.c		Directive processing

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	Many routines called "zd_xxx", to process directive ".xxx".

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */

#define __SEG__ Seg2

/* External routines */

extern int my_printf();
extern	long	objtell();
extern	SYMBOL	*symlookup();
extern	UBYTE	*zstr();
extern	STABLE	*symnew();
extern	SYMBOL	*symenter();
extern char **segstr();

extern TABLE_INFO *new_table();
extern TABLE_INFO *array_table();


/* External data */

extern	char	language_char;
extern	unsigned char *language_table;
extern	int	build_freq;
extern	char	Curfnm[];
extern	UWORD	Curline;
extern	long	Curseek;
extern	ODDISP	Dirtbl[];
extern	STABLE	*Gblsymtab;
extern	int	GvarC;
extern	STABLE	*Lclsymtab;
extern	LEXBUF	*LexP;
extern	UBYTE	*LextkP;
extern	int	ObjectC;
extern  int	VocC;
extern	int	Pass;
extern	UWORD	State;
extern	char	Svfnm[];
extern	UWORD	Svline;
extern	long	Svseek;
extern	LEXBUF	*SvlexP;
extern	UBYTE	*SvlextkP;
extern	long	Svobjseek;
extern	UWORD	Svstate;
extern	int	Version;
extern	int	Vocreclen;
extern	int Vockeylen;
extern	VNODE	*Voctrees[];
extern  long    funct_start;
extern  long    string_start;
extern  BOOL    yoffsets;

extern LGSYMBOL *cur_funct; /* Used for symbol table generation */
extern LGSYMBOL *cur_gstring;
extern BOOL		is_label;
extern TABLE_INFO *cur_table;
extern unsigned char table_status;
extern char	Zcsnorm[];

/* Local routines (and forward references) */


/* Local data */


/* Private data */

static	long	Tblloc;			/* Where table starts */
static	WORD	Tblsize;		/* Predicted size of table */

static	long	voc_start;  /* For symbol table */

	/* Translations of symbol type strings to type codes */
static struct {
    char	*tt_nameP;		/* Name string */
    int		tt_type;		/* Type code */
    } Stypetbl[] = {
	{ "FLAG",	FLAG_SPACE	},
	{ "NUMBER",	NUMBER_SPACE	},
	{ "FIX",        NUMBER_SPACE    },
 	{ "OBJECT",	OBJECT_SPACE	},
	{ "PROPERTY",	PROPERTY_SPACE	},
	{ "ROUTINE",	FUNCTION_SPACE	},
	{ "STRING",	ZSTRING_SPACE	},
	{ "TABLE",	TABLE_SPACE	},
	{ "VERB",	VERB_SPACE	},
	{ "ANY",        ANY_SPACE       },
	{ NULL,		ANY_SPACE	}
      };
/*
 
*//* zd_end

	.end
		End of assembly

*/

int
zd_end( dirnum )
	int		dirnum;
{
DREG1	int		sts;
		LGSYMBOL *sym;
		
    sym = (LGSYMBOL *)symenter( Gblsymtab, "FLAGS", sizeof(LGSYMBOL) );
	if ((sym->lg_val.v_flags & ST_DEFINED) == 0) {
		sym->lg_val.v_flags |= ST_DEFINED;
		sym->lg_val.v_value = 0;			/* Default value */
		sym->lg_space = CONSTANT_SPACE;
	      }
    if (yoffsets) {
      sym = (LGSYMBOL *)symlookup( Gblsymtab, "FOFF" );
      if (sym != NULL) {
	sym->lg_val.v_flags |= ST_DEFINED;
	sym->lg_val.v_value = funct_start >> 3;
	sym->lg_space = CONSTANT_SPACE;
      }
      sym = (LGSYMBOL *)symlookup( Gblsymtab, "SOFF" );
      if (sym != NULL) {
	sym->lg_val.v_flags |= ST_DEFINED;
	sym->lg_val.v_value = string_start >> 3;
	sym->lg_space = CONSTANT_SPACE;
      }
    }
    if ( ( sts = fixup( FALSE ) ) == SCNOTDONE )
	sts = SCEOF;

    return( sts );
}
/*
 
*//* zd_endi, zd_insert

	.insert	"filename"
		Insert file at this point

	.endi
		End of insert file

*/

int
zd_insert( dirnum )
	int		dirnum;
{
    /* All .insert directives should be removed by the lexicalizer */
    zerror( E_ALWAYS, "internal error - .insert present in token stream" );
    return( SCERR );
}


int
zd_endi( dirnum )
	int		dirnum;
{
    /* Valid .endi is processed by the lexicalizer. */
    zerror( E_PASS1, ".endi not in insert file." );
    return( assync( FALSE ) );
}
/*
 
*//* zd_true, zd_false

	.true
		generate a word with value 1

	.false
		generate a word with value 0

*/

int
zd_false( dirnum )
	int		dirnum;
{

    if ( assync( TRUE ) != SCOK )
	return( assync( FALSE ) );
	if (is_label)
		set_table(new_table(WORD_TYPE,BOOLEAN_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(WORD_TYPE,BOOLEAN_SPACE,2);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != WORD_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);
    return( objputw( 0 ) );
}


int
zd_true( dirnum )
	int		dirnum;
{

    if ( assync( TRUE ) != SCOK )
	return( assync( FALSE ) );
	if (is_label)
		set_table(new_table(WORD_TYPE,BOOLEAN_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(WORD_TYPE,BOOLEAN_SPACE,2);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != WORD_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);
    return( objputw( 1 ) );
}
/*
 
*//* zd_align

	.align val
		Align object code to multiple of val
		
*/

int
zd_align( dirnum )
	int 	dirnum;
{
	ZNUM	val;
	DREG2 int	sts;

    /* Get the argument */
    if ( ( sts = evalconst( &val ) ) != SCOK )
	return( sts );

    /* Align to val-byte boundary */
    if ( ( sts = objalign( val ) ) != SCOK )
	return( sts );
	
    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zd_byte, zd_word

	.byte	val [,val...]
		Output byte values

	[.word]	val [,val...]
		Output word values

*/

int
zd_byte( dirnum )
	int		dirnum;
{
extern	int		evalbyte();

	if (is_label)
		set_table(new_table(BYTE_TYPE,ANY_SPACE));
	if ((table_status == AT_TABLE_HEADER) ||
	    ((table_status == IN_TABLE) &&
		 (cur_table->it.array.el_type->type != BYTE_TYPE)))
		set_els(BYTE_TYPE,ANY_SPACE,1);
    return( zd_wdbyt( evalbyte ) );
}

int
zd_word( dirnum )
	int		dirnum;
{
extern	int		evalword();

	if (is_label)
		set_table(new_table(WORD_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(WORD_TYPE,ANY_SPACE,2);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != WORD_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);
    return( zd_wdbyt( evalword ) );
}


static int
zd_wdbyt( evalrtc )
AREG1	int		(*evalrtc)();	/* Evaluator routine to call */
{
DREG1	int		sts;

    for( ; ; ) {			/* Loop on values */
	/* Process the next term */
	if ( ( sts = (*evalrtc)() ) != SCOK )
	    return( sts );

	/* Look for another one, as indicated by comma */
	if ( ascomma() != SCOK )	/* Skip a comma */
	    break;			/* No comma */
    }

    if ( ( sts = assync( TRUE ) ) != SCOK )
	sts = assync( FALSE );
    return( sts );
}
/*
 
*//* zd_chrset


	.chrset id, n1,...n24[,n25][,n26]
	
	Redefine a character set. Id must be 0, 1 or 2.  If id is 0 or 1,
	the next 26 characters read become chars 6 thru 31 in that set. If id
	is 2, the next 24 characters read become chars 8 thru 31.
	
*/

int
zd_chrset( dirnum )
	int		dirnum;
{
		ZNUM	id,ch;
		int 	num,start;
	  DREG1 int     i;
	  DREG2 int     sts;

    /* Find the sort-record length */
    if ( !anyarg() )
	return( zexpected( E_PASS1, "char set id" ) );
    if ( ( sts = evalconst( &id ) ) != SCOK )
	return( sts );

	if ((id < 0) || (id > 2)) {
		zerror(E_PASS1,"Bad character set id in .chrset");
		return(SCERR);
	}
	switch (id) {
		case 0:
			start = 0;
			num = 26;
			break;
		case 1:
			start = 26;
			num = 26;
			break;
		case 2:
			start = 52;
			num = 26;
			break;
		default:
			break;
	}
	for (i = start; i < start + num; i++) {
		ascomma();				/* Skip any separator */
		if ( !anyarg() )
			return( zexpected( E_PASS1, "character" ) );
    	if ( ( sts = evalconst( &ch ) ) != SCOK )
			return( sts );
		if ((i != 52) && (i != 53))
		  Zcsnorm[i] = (char)ch;
	}
	zcset( &Zcsnorm[0] );


    /* Do end-of-line processing */
    assync( TRUE );
    return( assync( FALSE ) );
}

/*
 
*//* zd_fstr


	.fstr	name, string
		Define global name for string;  its value is the
		word number (i.e., address/2).
		Output string on word boundary;
 		add to list of frequent strings.

*/

int
zd_fstr( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	char		*snameP;	/* Ptr to symbol name */
AREG1	UBYTE		*strP;		/* Ptr to the string */
AREG2	LGSYMBOL	*symP;		/* General symbol ptr */

    /* Align fstr on w-byte boundary */
    if ( ( sts = objalign( 2 ) ) != SCOK )
	return( sts );

    /* Find the symbol name, and the string */
    if ( *LextkP != TKSYMBOL )
	return( zexpected( E_PASS1, "label for string" ) );
    memcpy( &snameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    ascomma();				/* Skip any comma */

    if ( *LextkP != TKSTRING )
	return( zexpected( E_PASS1, "string" ) );

    strP = LextkP+1;
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Define the name as a global symbol */
    symP = (LGSYMBOL *)symenter( Gblsymtab, snameP, sizeof( LGSYMBOL ) );
    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	zerror( E_PASS1, "duplicate symbol \"%s\"", snameP );
    else {
	symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL;
	symP->lg_loc = objtell();
	symP->lg_val.v_value = symP->lg_loc >> 1;
	symP->lg_space = FSTRING_SPACE;
    }

    /* Output the string and add it to the word table */
    if ( ( sts = addfstr( strP ) ) == SCOK ) {
	strP = zstr( strP, FALSE, 0 );	/* Translate to Z-string */

	if (is_label)
		set_table(new_table(ZSTRING_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(ZSTRING_TYPE,ANY_SPACE,1);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != ZSTRING_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

	/* Output the string */
	sts = objputzstr( strP );
    }

    return( sts );
}
/*
 
*//* zd_funct

	.funct	name[:type:minargs:maxargs],arglist
		Enter a function
	
	arglist = arg1[:type],arg2[:type],etc.

*/

int
zd_funct( dirnum )
	int		dirnum;
{
	int		i;
DREG1	int		sts;
DREG2	int		argC;		/* Number of function args */
	ZNUM		sv;		/* Scratch word value */
	char		*fnameP;	/* Ptr to function name */
	int			type;		/* Type returned by function */
	ZNUM		min,max;
	BOOL		arg_info = FALSE;
AREG1	LGSYMBOL	*symP;		/* General symbol ptr */
	char		*argname[FUNARGS];
	ZNUM		argval[FUNARGS];
	int			argtype[FUNARGS];

    /* Align function on quad- or 8-byte boundary */
    if (yoffsets && (funct_start == 0)) {
      if ( ( sts = objalign(8) ) != SCOK )
	return( sts );
      funct_start = objtell() - 256;
    } else
      if ( ( sts = objalign( 4 ) ) != SCOK )
	return( sts );

    /* Process the function definition */
    if ( *LextkP != TKSYMBOL )		/* Function must have a name */
	return( zexpected( E_PASS1, "name of function" ) );

    memcpy( &fnameP, LextkP+1, sizeof(char *) );  /* Get name */
    asnxtoken();			/* Skip past it */
	type = get_type(FALSE);	/* Look for function type after colon */
	
	if (ascolon() == SCOK) { /* get arg info? */
		arg_info = TRUE;
		if (!anyarg())
			return(zexpected(E_PASS1,"minimum args"));
		if ( ( sts = evalconst( &min ) ) != SCOK )
	  	  return( assync( FALSE ) );
		ascolon();
		if (!anyarg())
			return(zexpected(E_PASS1,"maximum args"));
		if ( ( sts = evalconst( &max ) ) != SCOK )
	  	  return( assync( FALSE ) );
	} 

    /* Collect the arguments */
    for( argC = 0; ascomma() == SCOK; ++argC ) {
	if ( *LextkP != TKSYMBOL )
	    break;			  /* Not a symbol, be done. */
	memcpy( &argname[argC], LextkP+1, sizeof(char *) );
	argval[argC] = 0;		  /* Init value */
	if ( asnxtoken() != SCOK )
	    continue;
	argtype[argC] = get_type(FALSE); /* Get type after colon */

	if ( ( *LextkP == TKCHAR ) && ( LextkP[1] == '=' ) ) {
	    /* Default value is supplied */
	    if ( Version != 4 )		/* Should be proper model */
		zerror( E_PASS1,
 		   "warning--default values not supported in version %d",
 		   Version );

	    if ( asnxtoken() != SCOK )
		continue;
	    if ( evalconst( &sv ) == SCOK )
		argval[argC] = sv;
	}
    }

    /* Define the function name as a global symbol */
    symP = (LGSYMBOL *)symenter( Gblsymtab, fnameP, sizeof( LGSYMBOL ) );
    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	zerror( E_PASS1, "duplicate function \"%s\"", fnameP );
    else {
	symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL;
	symP->lg_loc = objtell();
	symP->lg_add_info.function.start_page = symP->lg_loc / PAGE_SIZE;
	symP->lg_val.v_value = (symP->lg_loc - funct_start) >> 2;
	symP->lg_space = FUNCTION_SPACE;
	if (arg_info) {
		symP->lg_add_info.function.min_args = min;
		symP->lg_add_info.function.max_args = max;
	} else {
		symP->lg_add_info.function.min_args = 0; 	/* S.T. info  */
		symP->lg_add_info.function.max_args = argC; /* Min + max ?? */
	}
	symP->lg_add_info.function.value_space = type; 
	cur_funct = symP;
	if (yoffsets)
	  addfun(symP);
    }

    sts = objputb( argC );		/* Output arg count */
    if ( sts == SCOK ) {
	/* Output the arg default values && insert symbol names */
	for( i = 0; i < argC; ++i ) {
	    symP = ( LGSYMBOL *)symenter( Lclsymtab, argname[i],
						 sizeof( LGSYMBOL ) );
	    if ( ( Pass == 0 ) &&
		 ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
		zerror( E_PASS1, "duplicate argument name \"%s\"", argname[i] );
	    else {
		symP->lg_val.v_flags = ST_DEFINED|ST_VAR;
		symP->lg_val.v_value = i+1;
		symP->lg_space = VARIABLE_SPACE;
		symP->lg_add_info.variable.space = argtype[i];
	    }

	if ( Version == 4 )		/* Output default value */
	    if ( ( sts = objputw( argval[i] ) ) != SCOK )
		break;
	}
    }

    State |= AS_FUNCT;			/* Now processing function */

    assync( TRUE );			/* Check garbage on line */
    sts = assync( FALSE );

    /* Save assembly state in case we do another pass */
    strcpy( &Svfnm[0], &Curfnm[0] );
    Svline = Curline;
    Svseek = Curseek;
    SvlexP = LexP;
    SvlextkP = LextkP;
    Svstate = State;
    Svobjseek = objtell();

    return( sts );
}
/*
 
*//* zd_gstr

	.gstr	name, string
		Define global string (on quad boundary)

*/


int
zd_gstr( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	char		*snameP;	/* Ptr to the symbol name */
AREG1	UBYTE		*strP;		/* Ptr to the string */
AREG2	LGSYMBOL	*symP;		/* General symbol ptr */

    /* Find the symbol name, and the string */
    if ( *LextkP != TKSYMBOL )
	return( zexpected( E_PASS1, "label for string" ) );
    memcpy( &snameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    ascomma();				/* Skip any comma */

    if ( *LextkP != TKSTRING )
	return( zexpected( E_PASS1, "string" ) );
    strP = LextkP+1;
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    if (yoffsets && (string_start == 0)) {
      if ( ( sts = objalign(8) ) != SCOK )
	return( sts );
      string_start = objtell() - 256;
    } else
      if ( ( sts = objalign( 4 ) ) != SCOK )
	return( sts );

    /* Define the name as a global symbol */
    symP = (LGSYMBOL *)symenter( Gblsymtab, snameP, sizeof( LGSYMBOL ) );
    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	zerror( E_PASS1, "duplicate symbol \"%s\"", snameP );
    else {
	symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL;
	symP->lg_loc = objtell();
	symP->lg_add_info.string.start_page = symP->lg_loc / PAGE_SIZE;
	symP->lg_val.v_value = (symP->lg_loc - string_start) >> 2;
	symP->lg_space = ZSTRING_SPACE;
	cur_gstring = symP;
	if (yoffsets)
	  addstr(symP);
    }

	if (is_label)
		set_table(new_table(ZSTRING_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(ZSTRING_TYPE,ANY_SPACE,1);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != ZSTRING_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    /* Output the string */
    strP = zstr( strP, TRUE, 0 );	/* Translate to zstring */
    if ( ( sts = objputzstr( strP ) ) == SCOK ) {
        symP->lg_add_info.string.end_page = objtell() / PAGE_SIZE;
	assync( TRUE );			/* Check for stuff on line */
	sts = assync( FALSE );		/* Step to next line */
    }

    return( sts );
}
/*
 
*//* zd_gvar

	.gvar	name=value [,type]
		Define global variable.  Type is one of:  flag, number,
		  object, property, routine, string, table, verb.

*/

int
zd_gvar( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	int		type;
	char		*snameP;	/* Ptr to the symbol name */
AREG2	LGSYMBOL	*symP;		/* General symbol ptr */

    /* Make sure global variable count not exceeded */
    if ( GvarC > GVARMAX ) {
        zerror( E_PASS1, "too many global variables" );
	return( assync( FALSE ) );
    }

    /* Get the symbol name */
    if ( *LextkP != TKSYMBOL )
	return( zexpected( E_PASS1, "global variable name" ) );
    memcpy( &snameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    /* Get its value and output it to the object file */
    if ( ( *LextkP != TKCHAR ) || ( LextkP[1] != '=' ) )
	return( zexpected( E_PASS1, "\"=\"" ) );
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );
    if ( ( sts = evalword() ) != SCOK )
	return( sts );

    /* Get its type */
 	type = get_type(TRUE);

    /* Define the "gvar" as a global symbol */
    symP = (LGSYMBOL *)symenter( Gblsymtab, snameP, sizeof( LGSYMBOL ) );
    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	zerror( E_PASS1, "duplicate symbol \"%s\"", snameP );
    else {
	symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL|ST_VAR;
	symP->lg_val.v_value = GvarC++;
	symP->lg_space = VARIABLE_SPACE;
	symP->lg_add_info.variable.space = type;
	symP->lg_loc = objtell();
	
	if (is_label)
		set_table(new_table(WORD_TYPE,type));
	if (table_status == AT_TABLE_HEADER)
		set_els(WORD_TYPE,ANY_SPACE,2);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != WORD_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    }

    assync( TRUE );			/* end-of-line syntax check */
    return( assync( FALSE ) );		/* Skip to next line */
}
/*
 
*//* zd_len

	.len
		Doesn't do anything!!

*/

int
zd_len( dirnum )
	int		dirnum;
{
    assync( TRUE );			/* Check for garbage on line */
    return( assync( FALSE ) );		/* Go to next line */
}
/*
 
*//* zd_new

	.new	arg
		sets the release level of ZAP/ZIP.  arg is 3, 4, or 5.

*/

int
zd_new( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	ZNUM		level;

    /* Get the argument */
    if ( ( sts = evalconst( &level ) ) != SCOK )
	return( sts );

    /* This implementation only implements level 5 or 6 */
    if (( level != 5 ) && (level != 6))
	zerror( E_PASS1, "warning - unsupported level: %d", level );
    if ( ( level >= 4 ) && ( level <= 6 ) ) {
	Version = level;
	version_update();
	}
	
    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zd_object

	.object	name, arg1, arg2, ..., arg7
		Assigns the next object number to name.
		Defines an object and outputs arg1 - arg7 as words.
		For .new==3 arg3-arg4 are bytes, and arg7 doesn't exist.

	(Note - we're only implementing .new=5)

*/

int
zd_object( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	int		i;
	char		*onameP;	/* Object name */
AREG1	LGSYMBOL	*symP;		/* General symbol ptr */

    /* Pick up the symbol name */
    if ( *LextkP != TKSYMBOL )
	return( zexpected( E_PASS1, "object name" ) );
    memcpy( &onameP, LextkP+1, sizeof(char *) );
    if ( ( sts = asnxtoken() ) != SCOK )
      return( sts );

    /* Define the object as a global symbol */
    symP = (LGSYMBOL *)symenter( Gblsymtab, onameP, sizeof( LGSYMBOL ) );
    if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	zerror( E_PASS1, "duplicate symbol \"%s\"", onameP );
    else {
	symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL;
	symP->lg_val.v_value = ++ObjectC;
	symP->lg_space = OBJECT_SPACE;
	symP->lg_loc = objtell();
	
	if (is_label)
		set_table(new_table(OBJECT_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(OBJECT_TYPE,ANY_SPACE,OBJECT_SIZE);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != OBJECT_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    }

    /* Process the next 7 arguments as words */
    for( i = 0; i < 7; ++i ) {
	sts = ascomma();		/* Skip (maybe) comma */
	if ( ( sts != SCOK ) && ( sts != SCNOTDONE ) )
	    return( sts );
	if ( !anyarg() ) {
	    zerror( E_PASS1, "too few arguments" );
	    break;
	}

	if ( ( sts = evalword() ) != SCOK )
	    return( sts );
    }

    assync( TRUE );			/* Syntax check end of line */
    return( assync( FALSE ) );
}
/*
 
*//* zd_page

	.page	
		Align on TOPS-20 page boundary by outputting 0-value bytes.

*/

int
zd_page( dirnum )
	int		dirnum;
{
DREG1	int		sts;

    /* Align to next "page" */
    if ( ( sts = objalign( 512*4 ) ) != SCOK )
	return( sts );

    assync( TRUE );			/* Syntax check for end of line */
    return( assync( FALSE ) );		/* Sync to next line */
}
/*
 
*//* zd_pcset

	.pcset	newpc
		Align at newpc by outputting 0-value bytes.

*/

int
zd_pcset( dirnum )
	int		dirnum;
{
DREG2	int		sts;
	ZNUM		newpc;		/* New value for pc */
DREG1	ZNUM		pcdiff;		/* How far to go */

    if ( ( sts = evalconst( &newpc ) ) != SCOK )
	return( sts );

    pcdiff = newpc - objtell();		/* How far to go */
    while( pcdiff-- > 0 )
	if ( ( sts = objputb( 0 ) ) != SCOK )
	    return( sts );

    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zd_pdef

	.pdef
		Align at next word boundary.

*/

int
zd_pdef( dirnum )
	int		dirnum;
{
DREG1	int		sts;

    /* Align to word boundary */
    if ( ( sts = objalign( 2 ) ) != SCOK )
	return( sts );

    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zd_prop

	.prop	length, number
		Length and number are output as byte(s) with appropriate
		  bit setups.  Only allowed in a table.

*/

int
zd_prop( dirnum )
	int		dirnum;
{
DREG1	int		sts;
	ZNUM		propL;		/* Property length */
	ZNUM		propN;		/* Property number */

    /* Must be in a table */
    if ( ( State & AS_TABLE ) == 0 ) {
	zerror( E_PASS1, ".prop not in table" );
	return( assync( FALSE ) );
    }

    /* Get property length and property number */
    if ( !anyarg() )
	return( zexpected( E_PASS1, "property length" ) );
    if ( ( sts = evalconst( &propL ) ) != SCOK )
	return( sts );

    ascomma();				/* Skip any separator */

    if ( !anyarg() )
	return( zexpected( E_PASS1, "property number" ) );
    if ( ( sts = evalconst( &propN ) ) != SCOK )
	return( sts );

    /* Check the property number */
    if ( ( propN & ~0x3f ) != 0 ) {
	zerror( E_PASS1, "warning - bad property number" );
	propN = propN & 0x3f;
    }

    /* Output the property values in the correct format */
    if ( propL == 1 )
	sts = objputb( propN );
    else if ( propL == 2 )
	sts = objputb( propN | 0x40);
    else {
	if ( ( sts = objputb( propN | 0x80 ) ) == SCOK )
	    sts = objputb( propL | 0xc0 );
    }

	if (is_label)
		set_table(new_table(WORD_TYPE,PROPERTY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(WORD_TYPE,PROPERTY_SPACE,2);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != WORD_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    if ( sts == SCOK )
	assync( TRUE );

    return( assync( FALSE ) );
}
/*

*//* zd_segment, zd_endseg,zd_defseg

     .segment name
     .endseg
     .defseg name,aname1,aname2,aname3

     "Open" the segment with the given name.
     Close all segments.
     Make a segment "adjacent" to other segments.
*/

zd_segment(dirnum)
     int dirnum;
{
  DREG1	int		sts;
  AREG1	UBYTE		*strP;		/* Ptr to the string */

  /* Find the string */
  if ( *LextkP != TKSTRING )
    return( zexpected( E_PASS1, "string (segment name)" ) );
  strP = LextkP+1;
  if ( ( sts = asnxtoken() ) != SCOK )
    return( sts );
  segopen(*(segstr(strP)));
  assync( TRUE );			/* Check for stuff on line */
  sts = assync( FALSE );		/* Step to next line */
  return( sts );
}

zd_endseg(dirnum)
     int dirnum;
{
  segclose();
  return(SCOK);
}

zd_defseg(dirnum)
     int dirnum;
{
  DREG1	int		sts;
  AREG1	UBYTE		*strP;		/* Ptr to the segment */
  AREG2 UBYTE           *astrP = NULL;          /* Pointer to adjacent segment */
        ZNUM            start;

  /* Find the string */
  if ( *LextkP != TKSTRING )
    return( zexpected( E_PASS1, "string (segment name)" ) );
  strP = (UBYTE *)*(segstr(LextkP+1));
  if ( ( sts = asnxtoken() ) != SCOK )
    return( sts );
  if ((ascomma() != SCOK) || (*LextkP != TKINTEGER))
    return(zexpected(E_PASS1,"startup segment info (0 or 1)"));
  evalconst(&start);
  while(ascomma() == SCOK) {
    if ( *LextkP != TKSTRING)
      return(zexpected(E_PASS1,"string (segment name)"));
    astrP = (UBYTE *)*(segstr(LextkP+1));
    if ((sts = asnxtoken()) != SCOK)
      return (sts);
    addadj(strP,astrP);
  }
  if (start == 1)
    segstartup(strP);
  assync( TRUE );			/* Check for stuff on line */
  sts = assync( FALSE );		/* Step to next line */
  return( sts );
}

/*

*//* zd_seq

	.seq	[,name...]
		define each name with incrementing values starting at 0.

*/

int
zd_seq( dirnum )
	int		dirnum;
{
DREG1	int		sts;
DREG2	int		value;
	char		*nameP;		/* Ptr to symbol name */
AREG1	LGSYMBOL	*symP;		/* Ptr to symbol block */

    for( value = 0 ; ; ++value ) {	/* Loop through the list */
	sts = ascomma();		/* Skip any comma */
	if ( ( sts != SCOK ) && ( sts != SCNOTDONE ) )
	    return( sts );

	if ( *LextkP != TKSYMBOL )
	    break;			/* All done... */

	memcpy( &nameP, LextkP+1, sizeof(char *) );
	if ( ( sts = asnxtoken() ) != SCOK )
	    return( sts );

	/* Define the symbol */
	symP = (LGSYMBOL *)symenter( Gblsymtab, nameP, sizeof( LGSYMBOL ) );
	if ( ( Pass == 0 ) && ( ( symP->lg_val.v_flags & ST_DEFINED ) != 0 ) )
	    zerror( E_PASS1, "duplicate symbol \"%s\"", nameP );
	else {
	    symP->lg_val.v_flags = ST_DEFINED|ST_GLOBAL;
	    symP->lg_val.v_value = value;
	    symP->lg_space = OBJECT_SPACE;
	    symP->lg_loc = objtell();
	}
    }

    /* Check out the end of the line, and be done */
    assync( TRUE );
    return( assync( FALSE ) );
}
/*
 
*//* zd_str, zd_strl

	.str	string

	.strl	string
		Output a byte containing the length of the string in words,
		followed by the string in ZWORD format.

*/


int
zd_str( dirnum )
	int		dirnum;
{
	if (is_label)
		set_table(new_table(ZSTRING_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(ZSTRING_TYPE,ANY_SPACE,1);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != ZSTRING_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    return( dostr( FALSE ) );
}


int
zd_strl( dirnum )
	int		dirnum;
{
	if (is_label)
		set_table(new_table(BYTE_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER) {
		cur_table->type = PTABLE_TYPE;
		table_status = NOT_IN_TABLE;
	}
	else if (table_status == IN_TABLE)
		set_els(WORD_TYPE,ANY_SPACE,2);

    return( dostr( TRUE ) );
}


static int
dostr( lenF )
	int		lenF;		/* Whether to output string length */
{
DREG1	int		sts;
AREG1	UBYTE		*strP;		/* Ptr to the string */

    /* Find the string */
    if ( *LextkP != TKSTRING )
	return( zexpected( E_PASS1, "string" ) );
    strP = LextkP+1;
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    strP = zstr( strP, TRUE, 0 );	/* Translate to zstring */
    if (build_freq) return(SCOK);
    
    if ( lenF )				/* Output string length ? */
	if ( ( sts = objputb( zstrlen( strP ) ) ) != SCOK )
	    return( sts );

    /* Output the string */
    if ( ( sts = objputzstr( strP ) ) == SCOK ) {
	assync( TRUE );			/* Check for stuff on line */
	sts = assync( FALSE );		/* Step to next line */
    }

    return( sts );
}
/*
 
*//* zd_table, zd_endt

	.table	[length]
		Start a table.  Optional length if supplied must not be
		exceeded by table size (checked by .endt).

	.endt
		End table

*/

int
zd_table( dirnum )
	int		dirnum;
{

    /* Table definition can implicitly end the previous one */
    if ( ( State & AS_TABLE ) != 0 )
	endtable();

    /* See if a table size argument */
    if ( anyarg() ) {
	if ( evalconst( &Tblsize ) != SCOK )
	    return( assync( FALSE ) );
	if ( Tblsize < 0 ) {
	    zerror( E_PASS1, "warning - invalid table size -- ignored" );
	    Tblsize = -1;
	}
    }
    else
	Tblsize = -1;

    State |= AS_TABLE;
    Tblloc = objtell();
    assync( TRUE );
	
	if (is_label) {
		cur_table = array_table();
		set_table(cur_table);
		table_status = AT_TABLE_HEADER;
	}
    return( assync( FALSE ) );
}


int
zd_endt( dirnum )
	int		dirnum;
{
    if ( ( State & AS_TABLE ) == 0 )
	zerror( E_PASS1, "warning - .endt not in a table" );
    else {
	endtable();			/* Do end-table processing */
	assync( TRUE );			/* Report any junk on the line */
    }
    return( assync( FALSE ) );
}


/* Local routine to end a table */
static
endtable()
{
  long tsize;

    tsize = objtell() - Tblloc;
    if ( Tblsize != -1 ) {
	/* Table was defined with a size - must check it */
	if ( Tblsize != tsize )
	    zerror( E_PASS1,
		    "warning - table length does not match declared size" );
    }
    State &= ~AS_TABLE;
	if (table_status != NOT_IN_TABLE) {
		if (tsize % cur_table->it.array.el_size != 0)
			if (tsize % 2 == 0)
				set_els(WORD_TYPE,ANY_SPACE,2);
			else
				set_els(BYTE_TYPE,ANY_SPACE,1);
		cur_table->it.array.num_els = tsize / cur_table->it.array.el_size;
		table_status = NOT_IN_TABLE;
	}
}
/*
 
*//* zd_vocbeg, zd_vocend

	.vocbeg	record-length, key-length
		Begin vocabulary area.  Each datum from here to
		a ".vocend" is a record of `record-length' bytes whose first
		"key-length" bytes will be used as a sort key; the records
		will be sorted by that key before being output to the object file.
		The sorting is buried in the object data output
		process.

	.vocend
		End vocabulary area.

*/

int
zd_vocbeg( dirnum )
	int		dirnum;
{
DREG1	int		sts;
DREG2   int             i;
    /* Find the sort-record length */
    if ( !anyarg() )
	return( zexpected( E_PASS1, "record length" ) );
    if ( ( sts = evalconst( &Vocreclen ) ) != SCOK )
	return( sts );

    ascomma();				/* Skip any separator */

    if ( !anyarg() )
	return( zexpected( E_PASS1, "key length" ) );
    if ( ( sts = evalconst( &Vockeylen ) ) != SCOK )
	return( sts );
	
    if ( Vockeylen > Vocreclen)
      zerror( E_PASS1, "Vocab key length greater than record size");
    if ( Vockeylen < 1) {
      zerror(E_PASS1, "Vocab key size less than 1");
      return (sts);
    }
    State |= AS_VOCAB;
    for (i = 0; i < 256; Voctrees[i++] = NULL);

    if (table_status != NOT_IN_TABLE) {
      voc_start = objtell();
      set_up_vocab(voc_start - Tblloc);
    }
	

    /* Do end-of-line processing */
    assync( TRUE );
    return( assync( FALSE ) );

}


int
zd_vocend( dirnum )
	int		dirnum;
{
    long vocsize;

    if ( ( State & AS_VOCAB ) == 0 )
	zerror( E_PASS1, "warning - .vocend not in vocabulary area" );
    else {
	State &= ~AS_VOCAB;		/* No longer in vocab */

	objvocab( Voctrees );	/* Do end-table output */
	
	if (table_status == VOCAB_TABLE) {
	        vocsize = objtell() - voc_start;
		VocC += vocsize / Vocreclen;
		if (vocsize % Vocreclen != 0)
			zerror( E_PASS1, "warning - Vocab area bad length");
		cur_table->it.array.num_els = vocsize / Vocreclen;
		table_status = NOT_IN_TABLE;
	}

    }

    assync( TRUE );			/* Report any junk on the line */
    return( assync( FALSE ) );
}
/*
 
*//* zd_zword

	.zword	string
		Output a string, no frequency checking.

*/

int
zd_zword( dirnum )
	int		dirnum;
{
DREG1	int		sts;
AREG1	UBYTE		*strP;		/* Ptr to the string */

    /* Find the string */
    if ( *LextkP != TKSTRING )
	return( zexpected( E_PASS1, "string" ) );
    strP = LextkP+1;
    if ( ( sts = asnxtoken() ) != SCOK )
	return( sts );

    strP = zstr( strP, FALSE, 9 );	/* Translate to 9-zbyte zstring */
    
    /* Output the string */
    if ( ( sts = objputzstr( strP ) ) == SCOK ) {
	assync( TRUE );			/* Check for stuff on line */
	sts = assync( FALSE );		/* Step to next line */
    }
	if (is_label)
		if (table_status == VOCAB_TABLE)
			set_table(new_table(VWORD_TYPE,ANY_SPACE));
		else
			set_table(new_table(ZSTRING_TYPE,ANY_SPACE));
	if (table_status == AT_TABLE_HEADER)
		set_els(ZSTRING_TYPE,ANY_SPACE,ZWORD_SIZE);
	else if ((table_status == IN_TABLE) &&
			 (cur_table->it.array.el_type->type != ZSTRING_TYPE))
		set_els(WORD_TYPE,ANY_SPACE,2);

    return( sts );
}

zd_options(dirnum)
int dirnum;
{
  int sts;
  int do_ask, do_script;
  sts = evalconst(&do_script);
  if ((sts != SCOK) && (sts != SCNOTDONE))
    return(zexpected(E_PASS1, "number"));
  sts = ascomma();
  if ((sts != SCOK) && (sts != SCNOTDONE))
    return(sts);
  sts = evalconst(&do_ask);
  if ((sts != SCOK) && (sts != SCNOTDONE))
    return(zexpected(E_PASS1, "integer"));
  add_options_resource(do_script, do_ask);
  return(SCOK); }

zd_picfile(dirnum)
int dirnum;
{
  DREG1 int sts;
  UBYTE *file_name, *machine_name;
  ZNUM nums[4];
  int i;
  if (*LextkP != TKSTRING)
    return(zexpected(E_PASS1, "file name"));
  file_name = (UBYTE *)MALLOC(strlen(LextkP + 1));
  memcpy(file_name, LextkP + 1, strlen(LextkP + 1));
  asnxtoken();
  sts = ascomma();
  if ((sts != SCOK) && (sts != SCNOTDONE))
    return(sts);
  if (*LextkP != TKSTRING)
    return(zexpected(E_PASS1, "machine name"));
  machine_name = (UBYTE *)MALLOC(strlen(LextkP + 1));
  memcpy(machine_name, LextkP + 1, strlen(LextkP + 1));
  if ((sts != SCOK) && (sts != SCNOTDONE))
    return(sts);
  asnxtoken();
  for (i = 0; i < 4; i++) {
    ascomma();
    sts = evalconst(&nums[i]);
    if ((sts != SCOK) && (sts != SCNOTDONE))
      return(zexpected(E_PASS1, "number")); }
  assync(TRUE);
  add_zpcf_resource(file_name, machine_name, nums[0], nums[1], nums[2], nums[3]);
  return(SCOK); }
/*
 
*//* All the unimplemented directives

*/

unsigned char german_table[] = {'a', 155, 'o', 156, 'u', 157,
				  'A', 158, 'O', 159, 'U', 160,
				  's', 161, '>', 162, '<', 163, 0};

int zd_lang( dirnum )
int dirnum;
{
  int lang_id, lang_escape, sts;
  if (!anyarg())
    return(zexpected(E_PASS1, "language ID"));
  if ((sts = evalconst(&lang_id)) != SCOK)
    return(sts);
  ascomma();
  if (!anyarg())
    return(zexpected(E_PASS1, "escape character"));
  if ((sts = evalconst(&lang_escape)) != SCOK)
    return(sts);
  if (lang_id != 1)
    zerror(E_PASS1, "German and English are the only languages supported");
  language_char = lang_escape;
  language_table = german_table;
  my_printf("German characters; escape is %c.\n", language_char);
  assync(TRUE);
  return(assync(FALSE)); }

int zd_time( dirnum ) { return zd_nyi( dirnum ); }


zd_nyi( dirnum )
{
    zerror( E_ALWAYS, "%s: directive not yet implemented",   
	Dirtbl[dirnum].od_name );
    return( assync( FALSE ) );
}


/***
	get_type(after_comma)
	
	This function returns the type of an item, to be found after a comma
	or a colon, if one exists. The separator is a comma if after_comma
	is true, otherwise a colon.  Returned is the UBYTE representing the
	space of the item.  ANY_SPACE is default.
***/

get_type(after_comma)
	BOOL	after_comma;
{
	int sts;
	char *tnameP;
	int type,i;

    /* Get its type */
	if (after_comma)
  		sts = ascomma();			/* Maybe skip comma */
	else
		sts = ascolon();			/* or colon */
    if ( sts == SCNOTDONE )		/* If no comma or colon */
	type = ANY_SPACE;		/*  space unknown  */
    else if ( sts != SCOK )		/* Error? */
	return( sts );
    else {
	if ( *LextkP == TKSYMBOL )
	    memcpy( &tnameP, LextkP+1, sizeof(char *) );
	else if ( *LextkP == TKSTRING )
	    tnameP = (char *)LextkP+1;
	else
	    return( zexpected( E_PASS1, "type name" ) );
	if ( ( sts = asnxtoken() ) != SCOK )
	    return( sts );

	/* Lookup the type */
	for( i = 0, type = -1; Stypetbl[i].tt_nameP != NULL; ++i )
	    if ( strcmp( tnameP, Stypetbl[i].tt_nameP ) == 0 ) {
		type = Stypetbl[i].tt_type;
		break;
	    }

	/* Did not recognize type - give warning and default to any */
	if ( type == -1 ) {
	    zerror( E_PASS1, "unknown type \"%s\"", tnameP );
	    type = ANY_SPACE;
	}
	}
	
	/* return the type */
	return(type);
}
