/*	zfixup.c	Fixup of forward references

	1988 Zinn Computer Company, for Infocom


This file contains code dealing with fixups of forward references.
Contained are:

	fixup		Perform fixups
	makefixup	Make a fixup reference
	undef		Check undefined symbols

Also internal routine(s):

	fixupone	Process a single fixup block
	fix_syms	        Loop through a given fixup chain
	res_fix	        Reset a fixup chain for another pass
	undefsym	Check a given symbol for un-definition

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern	char	*MALLOC();

/* External data */

extern	int	build_freq;
extern	char	Curfnm[];
extern	UWORD	Curline;
extern	long	Curseek;
extern	ODDISP	Dirtbl[];		/* Data for directives */
extern	FILE	ErrfP;
extern	STABLE	*Gblsymtab;
extern	STABLE	*Lclsymtab;
extern	LEXBUF	*LexH;			/* Lex buffer header */
extern	LEXBUF	*LexP;			/* Current lex buffer pointer */
extern	UBYTE	*LextkP;		/* Lex token pointer */
extern	ODDISP	Octbl[];		/* Data for opcodes */
extern	int	Pass;
extern	UWORD	State;
extern	char	Svfnm[];
extern	UWORD	Svline;
extern	long	Svseek;
extern	LEXBUF	*SvlexP;
extern	UBYTE	*SvlextkP;
extern	long	Svobjseek;
extern	UWORD	Svstate;

extern LGSYMBOL *cur_label;
extern FIXUP    *loc_chain;
extern FIXUP    *glo_chain;

extern	VNODE	*CurvocP;
extern	int	VocX;

/* Local routines (and forward references) */

	int	fixupsym();
	int	resetsym();
	int	undefsym();

/* Local data */

static	int	Fixupsts;		/* Status from fixup */
static	char	Shortening;		/* If we are shortening... */

/* Private data */

/*
 
*//* fixup( shortenF )

	Process all fixup information


Accepts :

	shortenF	TRUE if a re-pass will be done as a result of
			  instruction shortening;
			FALSE if not

Returns :

	<value>		Status code:
			  SCOK if another pass is being started
			  SCNOTDONE if no instructions were shortenable;
			  other if error occured (??)

*/

int fixup( shortenF )
int shortenF;
{
  DREG1 int sts;
  long curaddr;
  FIXUP *tmp = glo_chain;
  Shortening = shortenF;		/* Remember if we will repass */
  curaddr = objtell();		/* Remember where we are */
  Fixupsts = SCNOTDONE;		/* Init status */
  /* Only look at the global symbols if absolutely necessary, as
     it could (should, too) be a major waste of time otherwise.  Thus
     only look at it if end-of-assembly processing. */
  
  if (Shortening)
    fix_syms(&loc_chain);      /* Do predicate table */
  else
    fix_syms(&glo_chain);	/* Do global table */
  if (Fixupsts != SCOK) {		/* Don't need another pass */
    objseek(curaddr, FALSE);	/* Get back to where we left off */
    Pass = 0;			/* Not another pass */
    SvlexP = NULL;			/* No saved point */
    return(Fixupsts);		/* Return the condition */ }
  /* Another pass needs to be done.  Setup it so it will magically
     continue where we left off. */
  if ((sts = objseek( Svobjseek, TRUE)) != SCOK)
    return(sts);			/* Can't reposition object file! */
  strcpy(&Curfnm[0], &Svfnm[0]);
  Curline = Svline;
  Curseek = Svseek;
  LexP = SvlexP;
  LextkP = SvlextkP;
  State = Svstate;
  /* Touch up all the symbol table entries */
    if ((sts = res_fix(&loc_chain)) == SCOK)
      sts = res_fix(&glo_chain);
  ++Pass;				/* Flag non-first pass */
  cur_label = NULL;   /* Needed to avoid accidental half-kill */
  return(sts);			/* Tell caller another pass */
}
/*
 
*//* fix_syms( fix_list )

     This procedure loops through a fixup chain, fixing up each unresolved
forward reference.

	Fixupsts	Status code:
			  SCOK if another pass is to be done
			  SCNOTDONE if no instructions were shortenable;
			  other if error occured (??)

			  
*/

static int fix_syms( fix_list )
FIXUP **fix_list;
{
  DREG1 int sts;
  AREG1 FIXUP **fixP;
  AREG2 FIXUP *fix;
  for(fixP = fix_list; (fix = *fixP) != NULL; )
    if ((fix->fx_sym->lg_val.v_flags & ST_DEFINED) != 0) {
      *fixP = fix->fx_next;
      sts = fixupone(fix);
      /* Integrate status... */
      if (sts == SCOK)		/* Shrink might have happened */
	Fixupsts = SCOK;
      else if (sts != SCNOTDONE)
	return(sts);		/* Abort!  abort! */
      FREE(fix); }
    else {
      zerror(E_ALWAYS,"Undefined symbol %s in fixup",fix->fx_sym->lg_sym.sym_name);
      /* Used to return, leading to completely useless ZIP files. */
      *fixP = fix->fx_next;
      FREE(fix); }
  return(SCOK);
}

/*
 
*//* fixupone( fix )

	Process a particular fixup block

Accepts :

	fix		Address of the fixup block

Returns :

	<value>		Status code:
			  SCOK if any instructions could have been shortened;
			  SCNOTDONE if no instructions were shortenable;
			  other if error occured (??)

*/

static int fixupone(fix)
AREG1	FIXUP		*fix;		/* The fixup block */
{
  AREG2   LGSYMBOL        *sym;
  DREG1	int		sts;
  DREG2	UWORD		value;
  
  sym = fix->fx_sym;
/*  if (strcmp(sym->lg_sym.sym_name, "W?NORTH") == 0)
    printf("Fixup of W?NORTH\n"); */
  value = sym->lg_val.v_value + fix->fx_offset;

  switch(fix->fx_class) {	/* Process by type */
  case FXBYTE:			/* Straight byte reference */
    if ((value & ~0xff) != 0)
      zerror(E_ALWAYS, "forward reference to %s does not resolve to byte",
	     sym->lg_sym.sym_name);
    objseek(fix->fx_addr, FALSE);
    if ((sts = objputb(value)) == SCOK)
      sts = SCNOTDONE;
    break;
  case FXWORD:			/* Straight word reference */
    objseek(fix->fx_addr, FALSE);
    if ((sts = objputw(value)) == SCOK)
      sts = SCNOTDONE;
    break;
  case FXFPREDS:			/* Short false forward predicate ref */
    objseek(fix->fx_addr, FALSE);
    value = sym->lg_loc - (fix->fx_addr + 1) + 2;
    if ((sts = objputb(value | 0x40)) == SCOK)
      sts = SCNOTDONE;
    break;
  case FXTPREDS:			/* Short true forward predicate ref */
    objseek(fix->fx_addr, FALSE);
    value = sym->lg_loc - (fix->fx_addr + 1) + 2;
    if ((sts = objputb(0xc0 | value)) == SCOK)
      sts = SCNOTDONE;
    break;
  case FXFPREDL:			/* Long false forward predicate ref */
    objseek(fix->fx_addr, FALSE);
    value = sym->lg_loc - (fix->fx_addr + 2) + 2;
    if ((sts = objputw(value)) == SCOK) {
      if (value > 0x3f)	/* If can't be shortened */
	sts = SCNOTDONE;	/*  indicate so */
      else if (Shortening)	/* Can; if we'll make another pass, */
	--sym->lg_loc;	/*  adjust the symbol location */ }
    break;
  case FXTPREDL:			/* Long true forward predicate ref */
    objseek(fix->fx_addr, FALSE);
    value = sym->lg_loc - (fix->fx_addr + 2) + 2;
    if ((sts = objputw(value | 0x8000)) == SCOK) {
      if (value > 0x3f)	/* If can't be shortened */
	sts = SCNOTDONE;	/*  indicate so */
      else if (Shortening)	/* Yes; if we'll make another pass */
	--sym->lg_loc;	/*  adjust the symbol location */ }
    break;
  case FXJUMP:			/* Relative jump */
    objseek(fix->fx_addr, FALSE);
    value = sym->lg_loc - (fix->fx_addr + 2) + 2;
    sts = objputw(value);	/* Output the relative value */
    if (sts == SCOK)
      sts = SCNOTDONE;	/* No shortening of jumps */
    break;
  default:
    zerror(E_ALWAYS, "internal error - unknown fixup class %d",	fix->fx_class);
    sts = SCERR;
    break; }
  return(sts);
}
/*
 
*//* res_fix( fix_list )

	Reset fixup characteristics for another pass.  Loops through a fixup chain.

Accepts :

       fix_list               List of fixups

Returns :

	<value>		status code
*/

static int
res_fix( fix_list )
	FIXUP	**fix_list;		/* Addr of fixup list */
{
AREG2	FIXUP		*fixP;		/* Fixup block ptr */
AREG3	FIXUP		**fixPP;	/* Fixup block list link addr */

    /* Remove any fixup entries created after the repeat-pass point */
    for( fixPP = fix_list; ( fixP = *fixPP ) != NULL; ) {
	if ( fixP->fx_addr >= Svobjseek ) {
	    /* Remove this one */
	    *fixPP = fixP->fx_next;
	    FREE( fixP );
	}
	else
	    break;
    }

return( SCOK );
}

/*
 
*//* makefixup( symP, class, addr, off )

	Make a fixup reference

Accepts :

	symP		Address of symbol table entry to tie reference to
	class		The class of fixup being made
	addr		The address where the fixup should occur
	off             Offset to add to symbol value

Returns :

	< value >	status code

*/

int makefixup(symP, class, addr, off)
AREG1	LGSYMBOL	*symP;
int		class;
long		addr;
ZNUM            off;
{
  AREG2	FIXUP		*fixP;
/*  if (strcmp(symP->lg_sym.sym_name, "W?NORTH") == 0)
    printf("Reference to W?NORTH\n"); */
  if (build_freq) return(SCOK);
  if ((fixP = (FIXUP *)MALLOC(sizeof(FIXUP))) == NULL) {
    zerror(E_ALWAYS, "can not allocate fixup block in makefixup()");
    return(SCMEMORY); }
  if ((State & AS_VOCAB) != 0) { /* We're building vocabulary */
    fixP->fx_next = CurvocP->vn_fixups;	/* So just chain this on the current node */
    CurvocP->vn_fixups = fixP; } /* Addr is actually VocX; becomes absolute later */
  else if ((symP->lg_val.v_flags & ST_GLOBAL) == 0) {
    fixP->fx_next = loc_chain;
    loc_chain = fixP; }
  else {
    fixP->fx_next = glo_chain;
    glo_chain = fixP; }
  fixP->fx_sym = symP;
  fixP->fx_class = class;
  fixP->fx_addr = addr;
  fixP->fx_offset = off;
  return(SCOK);
}

/*
 
*//* undef()

	Report undefined global symbols

Accepts :


Returns :

	<value>		Status code:
			  SCOK if another pass is being started
			  SCNOTDONE if no instructions were shortenable;
			  other if error occured (??)

*/

int
undef()
{
    symwalk( Gblsymtab, undefsym );	/* Look through global symbols */
}
/*
 
*//* undefsym( symP )

	Check a given global symbol to see if it is undefined.

Accepts :

	symP		Address of the symbol table entry

Returns :

	<value>		Status code;

Notes :

	This routine is called from the symwalk() symbol table
walking function.

*/

static int
undefsym( symP )
AREG1	LGSYMBOL	*symP;
{
    /* If the symbol is undefined, print it. */
    if ( ( symP->lg_val.v_flags & ST_DEFINED ) == 0 )
	zerror( E_ALWAYS, "\"%s\" undefined.", symP->lg_sym.sym_name );

    return( SCOK );
}
