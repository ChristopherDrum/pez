/*	zasm.c		Assembler driver

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	assemble	Assemble a file.
	asline		Assemble a source statement.

*/

#include <stdio.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern ST_SYMTAB	*sym_dump();

/* External data */

extern	int	build_freq;
extern	char	Curfnm[];
extern	UWORD	Curline;
extern	long	Curseek;
extern	ODDISP	Dirtbl[];		/* Data for directives */
extern	FILE	ErrfP;
extern	STABLE	*Lclsymtab;
extern	LEXBUF	*LexH;			/* Lex buffer header */
extern	LEXBUF	*LexP;			/* Current lex buffer pointer */
extern	UBYTE	*LextkP;		/* Lex token pointer */
extern	ODDISP	Octbl[];		/* Data for opcodes */
extern	BOOL	Opsrcerr;
extern	UWORD	State;
extern	LEXBUF	*SvlexP;

extern	long	S_lexsize;

extern LGSYMBOL	*cur_funct;		/* Used for symbol table generation */
extern LGSYMBOL *cur_label;
extern unsigned char num_labels;
extern BOOL		is_label;

extern int		Version;


/* Local routines (and forward references) */


/* Local data */


/* Private data */

/*
 
*//* assemble( filename )

	Drives the assembly for the specified source file

Accepts :

	filename	Name of file to assemble

Returns :

	<value>		Status cude

*/

int
assemble( filename )
	char		*filename;
{
	int		sts;
	int		lxX;		/* Scratch lex index */

    LexH = LexP = NULL;			/* No lex here */
    if ( ( sts = tklex( &LexP, &lxX, filename ) ) != SCOK )
	return( sts );

    /* Much of the assembly may have been instigated by the lex process --
       but we also need to finish it up here */

    while( asline() == SCOK )		/* Do each line */
	;
    return(sts);
}
/*
 
*//* asline()

	Assemble a source statement

Accepts :


Returns :

	<value>		SCOK to continue;
			 something else to stop.

Notes :

	It is assumed that a complete statement is handled here.
That is, state is at the beginning of a statement.

*/

int
asline()
{
	UWORD		w;		/* Scratch */
DREG1	int		tktype;
DREG2	int		sts;

    /* Process a single source line */

    sts = SCOK;				/* Assume OK */
    tktype = *LextkP;			/* Get current token */
    switch ( tktype ) {			/* Process it ! */
	case TKFILE:			/* New source file */
	    strcpy( &Curfnm[0], ++LextkP );
	    LextkP += ( strlen( LextkP ) +1);
	    break;

	case TKLINE:			/* Source line tracking */
	    memcpy( &Curline, ++LextkP, sizeof(UWORD) );
	    if ( Opsrcerr ) {		/* If source seek is attached */
		memcpy( &Curseek, LextkP+sizeof(UWORD), sizeof(long) );
		LextkP += sizeof(UWORD) + sizeof(long);
	    }
	    else			/* Seek info is not attached */
		LextkP += sizeof(UWORD);
	    break;
	    
	case TKLABEL:			/* Various label forms */
	case TKGLABEL:
	    sts = aslabel();
	    break;

	case TKEQUATE:			/* Equate */
	    sts = asequate();
	    break;

	case TKDIRECTIVE:		/* Assembler directive */
	    /* Any directive ends a function definition.  Ending a function
	       causes relocation fixups to be performed, which can in turn
	       cause a repeat pass over the previous function if any
	       instruction shortening can be done. */
	    if ( ( State & AS_FUNCT ) != 0 ) {
		State &= ~AS_FUNCT;	/* Clear function membership */

		/* Perform relocation fixups */
		if (build_freq)
		  sts = SCNOTDONE;
		else
		  sts = fixup( TRUE );

		/* If not making another pass, output and clear the symbol table. */
		if ( sts != SCOK ) {
		  cur_funct->lg_add_info.function.end_page = objtell() / PAGE_SIZE;
		  cur_funct->lg_add_info.function.syms = 
				            sym_dump(Lclsymtab,cur_funct->lg_loc, 0);
		    symempty( Lclsymtab, sizeof(LGSYMBOL) );
		}

		/* Check on the fixup result */
		if ( sts != SCNOTDONE )
		    break;		/* Another pass, or error */
	    }

	    w = LextkP[1];		/* Get directive number */
		/* Set flag if directive is at a label */
		if (num_labels == 0)
			is_label = FALSE;
		else
			is_label = (cur_label->lg_val.v_value == objtell());
	    if ( ( sts = asnxtoken() ) == SCOK )
		sts = (*Dirtbl[w].od_disp)( w );
	    break;

	case TKOPCODE:			/* Assembler instruction */
	    /* Opcodes are only valid in functions */
	    if ( ( State & AS_FUNCT ) == 0 ) {
		zerror( E_PASS1, "not in a function body" );
		sts = SCNOTDONE;
	    }
	    else {
		memcpy( &w, LextkP+1, sizeof(UWORD) );
		if (((Octbl[w].od_flags & ST_YZIP) != 0) &&
			(Version < YZIP))
			zerror(E_PASS1,"Opcode not available in pre-YZIP\n");
		if ( ( sts = asnxtoken() ) == SCOK )
		    sts = (*Octbl[w].od_disp)( &Octbl[w] );
	    }
	    break;

	case TKSYMBOL:			/* Default .word assembly */
	case TKINTEGER:
		/* Set flag if at a label */
		if (num_labels == 0)
			is_label = FALSE;
		else
			is_label = (cur_label->lg_val.v_value == objtell());
	    sts = zd_word( 0 );		/* Pretend .word */
	    break;

	case TKEOL:			/* No-op */
	    ++LextkP;
	    break;

	case TKEOB:			/* End of lexical buffer */
	    sts = asnxtoken();		/* Handle it down there */
	    break;

	case TKEOF:			/* End of source */
	    sts = zd_end( 0 );		/* Process end of assembly */
	    break;

	case TKSTRING:			/* Error to have these here */
	    zerror( E_PASS1, "unexpected literal string");
	    LextkP += ( strlen( LextkP+1 ) +2 );
	    assync( FALSE );
	    break;

	case TKCHAR:
	    ++LextkP;
	    zerror( E_PASS1,
		    "unexpected character 0x%x (%c)", *LextkP, *LextkP );
	    ++LextkP;
	    assync( FALSE );
	    break;

	case TKOTHER:
	    zerror( E_PASS1, "unexpected thing \"%s\"", ++LextkP );
	    LextkP += ( strlen( LextkP ) +1 );
	    assync( FALSE );		/* Synch to next line */
	    break;

	default:
	    zerror( E_ALWAYS,
		    "unknown token %d -- asline() internal error!!", tktype );
	    sts = SCEOF;
	    break;
    }

    /* Check for "not done" status; clean up line if that's it */
    if ( sts == SCNOTDONE )
	sts = assync( FALSE );

    return( sts );
}
/*
 
*//* ascomma()

	Skip a comma

Accepts :


Returns :

	<value>		SCOK if a comma was present and was skipped;
			SCNOTDONE if no comma was present
			other error code if error happened

*/

int
ascomma()
{
    if ( ( *LextkP == TKCHAR ) &&
	 ( LextkP[1] == ',' ) )
	return( asnxtoken() );
    return( SCNOTDONE );
}


/* ascolon()

	Skip a colon 

Accepts :


Returns :

	<value>		SCOK if a colon was present and was skipped;
			SCNOTDONE if no colon was present
			other error code if error happened

*/

int
ascolon()
{
    if ( ( *LextkP == TKCHAR ) &&
	 ( LextkP[1] == ':' ) )
	return( asnxtoken() );
    return( SCNOTDONE );
}


/*
 
*//* assync( errF )

	Syncronize token input to beginning of next line

Accepts :

	errF		If TRUE, reports an error if there's stuff
			  on the line after this.

Returns :

	<value>		status - SCOK to continue, other to stop


Notes -

	Skips to the next LINE or FILE token.

	Pass errF to pre-check for junk on line.

*/

int
assync( errF )
	int		errF;
{
DREG1	int		tktype;
DREG2	int		sts;

    for( ; ; ) {
        tktype = *LextkP;
	switch( tktype ) {
	    case TKLINE:		/* Things that make us return */
	    case TKFILE:
	    case TKEOF:
		return( SCOK );

	    default:			/* Skip over it */
	        sts = asnxtoken();
		break;
	}

	if ( sts != SCOK )
	    return( sts );

	if ( errF ) {
	    zerror( E_PASS1, "unexpected stuff on line." );
	    return( SCERR );
	}
    }
}
/*
 
*//* asnxtoken()

	Skip to next reasonable token

Accepts :


Returns :

	<value>		SCOK to continue;
			 something else to stop.

Notes :

	This routine advances to the next significant token.
The only really "insigificant" token is TKEOB, the link to the
next token block.  This routine handles this (and any other
uninteresting thing that may come along) before returning to
the caller.

*/

int
asnxtoken()
{
DREG1	int		tktype;
DREG2	int		skipF;		/* If already skipped */
AREG1	LEXBUF		*lexP;		/* For freeing lex buffers */

    skipF = FALSE;			/* Haven't skipped yet */
    for( ; ; ) {			/* Until we've skipped */
	tktype = *LextkP;		/* Get current token */
	switch ( tktype ) {
	    case TKFILE:		/* String attached to token */
	    case TKSTRING:
	    case TKOTHER:
		if ( skipF )		/* Skipped? */
		    return( SCOK );	/* Yup, return this thing */
		skipF = TRUE;		/* Now we've skipped... */
		LextkP += ( strlen( LextkP+1 ) +2);
		break;

	    case TKLABEL:		/* Literal string */
	    case TKGLABEL:
	    case TKEQUATE:
	    case TKSYMBOL:
	        if ( skipF )
		    return( SCOK );
		LextkP += (sizeof(char *) +1);
		skipF = TRUE;
		break;

	    case TKOPCODE:		/* WORD or UWORD attached */
		if ( skipF )		/* Skipped to here? */
		    return( SCOK );
		skipF = TRUE;
		LextkP += (sizeof(WORD) +1);
		break;

	    case TKINTEGER:		/* ZNUM attached */
		if ( skipF )		/* Skipped to here? */
		    return( SCOK );
		skipF = TRUE;
		LextkP += (sizeof(ZNUM) +1);
		break;

	    case TKCHAR:		/* Single byte attached */
	    case TKDIRECTIVE:
		if ( skipF )		/* Stop here? */
		    return( SCOK );
		skipF = TRUE;		/* stop next time... */
		LextkP += 2;
		break;

	    case TKEOL:			/* Nothing attached */
		if ( skipF )
		    return( SCOK );
		skipF = TRUE;
		++LextkP;
		break;

	    case TKLINE:		/* Source line tracking */
		if ( skipF )
		    return( SCOK );
		skipF = TRUE;
		if ( Opsrcerr )
		    LextkP += sizeof(UWORD) + sizeof(long) +1;
		else
		    LextkP += sizeof(UWORD) +1;
		break;

	    case TKEOB:			/* End of lexical buffer */
		LexP = LexP->lx_link;
		LextkP = &(LexP->lx_data[0]);

		/* Now we can free buffers up to here, or up to the
		   buffer containing the saved context for multiple
		   passes, whichever is appropriate */
		while( ( LexH != LexP ) && ( LexH != SvlexP ) ) {
		    lexP = LexH;
		    LexH = LexH->lx_link;
		    FREE( lexP );
		    S_lexsize -= sizeof(LEXBUF);
		}
		skipF = TRUE;		/* return on next legit token */
		break;			/* Keep going! */

	    case TKEOF:			/* End of source */
		return( SCOK );		/* Don't skip past EOF */
		break;

	    default:
		zerror( E_ALWAYS,
			 "unknown token %d -- asnxtoken() internal error!!",
				 tktype );
		return( SCERR );
		break;
	}
    }
}
