/*	zdata.c		Global data for ZAP

	1988 Zinn Computer Company, for Infocom


This file contains global data definitions.

*/

#include <stdio.h>

#include "zap.h"


	/* Routines that handle assembler directives */

extern	int	zd_align(),
		zd_byte(),
		zd_chrset(),
                zd_defseg(),
		zd_end(),
		zd_endi(),
                zd_endseg(),
		zd_endt(),
		zd_false(),
		zd_fstr(),
		zd_funct(),
		zd_gstr(),
		zd_gvar(),
		zd_insert(),
		zd_lang(),
		zd_len(),
		zd_new(),
		zd_object(),
  		zd_options(),
		zd_page(),
		zd_pcset(),
		zd_pdef(),
  		zd_picfile(),
		zd_prop(),
                zd_segment(),
		zd_seq(),
		zd_str(),
		zd_strl(),
		zd_table(),
		zd_time(),
		zd_true(),
		zd_vocbeg(),
		zd_vocend(),
		zd_word(),
		zd_zword();


	/* Routines that handle opcode processing */

extern	int	zo_0op(),
		zo_1op(),
		zo_2op(),
		zo_xop(),
		zo_printi(),
		zo_jump(),
		zo_call(),
		zo_icall();
	unsigned char	*language_table = 0;
	char	language_char = 0;
	int	build_freq = 0;	/* if true (-f flag) we're building new fwords table */
	char	Curfnm[100];		/* Name of current file */
	UWORD	Curline;		/* Current source line number */
	long	Curseek;		/* Seek for current source line */
	FILE	*Error_out = NULL; /* .errors file */
	FILE	*ErrfP = NULL;		/* Source file open for error msgs */
	char	Errfnm[100];		/* Source file last opened for error */
	UWORD	Errline;		/* Last error line */
	STABLE	*Gblsymtab;		/* Global symbols table */
	int	GvarC;			/* Global variable # */
	STABLE	*Lclsymtab;		/* Local symbols table */
	LEXBUF	*LexH;			/* Lex buffer header */
	LEXBUF	*LexP;			/* Current lex buffer */
	UBYTE	*LextkP;		/* Current token pointer */
	STABLE	*Litsymtab;		/* Literal string table */
        STABLE  *Segsymtab;             /* Segment name table */
	int	ObjectC;		/* Object counter */
	long	Objtop = 0;	/* Top of object file */
	STABLE	*Odsymtab;		/* Ptr to opcode/directive symtbl */
	BOOL	Opsrcerr = TRUE;	/* Print source line on errors */
	int	Pass;			/* Pass number */
	BOOL	Permshift = FALSE;	/* Can do permanent shifts */
	UWORD	State;			/* Assembler state flags */
        int	Version = VERS;         /* ZAP version (.new value) */
	int	Vocreclen = 0;		/* Vocabulary info record length */
	int Vockeylen = 0;		/* Vocabulary key length for sorting */
	VNODE	*Voctrees[256];	        /* Vocabulary record sort tree */
	VNODE	*CurvocP;	/* Current vocab node */
	int	VocX;		/* Index into current vocab node */
	int	VocC = 0;	/* Number of vocabulary words */
	int	Max_buf_len = STRINGBUF;  /* Current longest string */
	UWORD	Release = 0;			/* Release number */
        BOOL    yoffsets = FALSE;       /* Use YZIP offsets, for large games */
        FIXUP   *loc_chain = NULL;
        FIXUP   *glo_chain = NULL;
        UWORD   top_page;
	int	character_count = 0;
long character_counts[256];

	/* Symbol table generation stuff */
	
ST_SYMTAB *glbl_tab = NULL;
LGSYMBOL *cur_funct = NULL;
TABLE_INFO *cur_table = NULL;
unsigned char table_status = NOT_IN_TABLE;
LGSYMBOL *cur_label = NULL;
LGSYMBOL **label_list;
unsigned char num_labels = 0;
LGSYMBOL *cur_gstring = NULL;
BOOL is_label;
UBYTE cur_type;

long funct_start = 0;
long string_start = 0;

	/* Statistics stuff */

	long	S_lexmax = 0;		/* Max lex buffer space used */
	long	S_lexttl = 0;		/* Total lex buffer space allocated */
	long	S_lexsize = 0;		/* Lex buffer space used */
	long	S_objdata = 0;		/* Object data size */
	long	S_objsize = 0;		/* Object buffer space used */
	long	S_srcsize = 0;		/* Number of bytes of source read */

	/* Saved context to be able to go back for another pass */

	char	Svfnm[100];		/* Current filename */
	UWORD	Svline;			/* Line number */
	long	Svseek;			/* Seek address */
	LEXBUF	*SvlexP;		/* Lex buffer address */
	UBYTE	*SvlextkP;		/* Current token pointer */
	long	Svobjseek;		/* object file position (& PC) */
	UWORD	Svstate;		/* Assembly state */



	/* Normal Zap character set */
	char	Zcsnorm[] = {
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
'\0', '\n', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.',
',', '!', '?', '_', '#', '\'', '"', '/', '\\', '-', ':', '(', ')'
			     };

	/* Definition/dispatch table for assembler directives */

	ODDISP	Dirtbl[] = {
{ ".ALIGN",	0,		0,	zd_align },
{ ".BYTE",	0,		0,	zd_byte },
{ ".CHRSET",	0,		0,      zd_chrset },
{ ".DEFSEG",    0,              0,      zd_defseg },
{ ".END",	0,		0,	zd_end },
{ ".ENDI",	ST_ENDI,	0,	zd_endi },
{ ".ENDSEG",    0,              0,      zd_endseg },
{ ".ENDT",	0,		0,	zd_endt },
{ ".VOCEND",    0,		0,	zd_vocend },
{ ".FALSE",	0,		0,	zd_false },
{ ".FSTR",	0,		0,	zd_fstr },
{ ".FUNCT",	0,		0,	zd_funct },
{ ".GSTR",	0,		0,	zd_gstr },
{ ".GVAR",	0,		0,	zd_gvar },
{ ".INSERT",	ST_INSERT,	0,	zd_insert },
{ ".LANG",	0,		0,	zd_lang },
{ ".LEN",	0,		0,	zd_len },
{ ".NEW",	0,		0,	zd_new },
{ ".OBJECT",	0,		0,	zd_object },
{ ".OPTIONS",	0,		0,	zd_options},
{ ".PAGE",	0,		0,	zd_page },
{ ".PCSET",	0,		0,	zd_pcset },
{ ".PDEF",	0,		0,	zd_pdef },
{ ".PICFILE",	0,		0,	zd_picfile},
{ ".PROP",	0,		0,	zd_prop },
{ ".SEGMENT",   0,              0,      zd_segment },
{ ".SEQ",	0,		0,	zd_seq },
{ ".STR",	0,		0,	zd_str },
{ ".STRL",	0,		0,	zd_strl },
{ ".TABLE",	0,		0,	zd_table },
{ ".TIME",	0,		0,	zd_time },
{ ".TRUE",	0,		0,	zd_true },
{ ".VOCBEG",    0,		0,	zd_vocbeg },
{ ".WORD",	0,		0,	zd_word },
{ ".ZWORD",	0,		0,	zd_zword },
{ NULL,		0,		0,	NULL }
			};


	/* Definition/dispatch table for opcodes.  Data is
	   the opcode value; dispatch is the general handler. */


	ODDISP	Octbl[] = {
{ "ADD",	ST_VAL,		20,	zo_2op },
{ "ASHIFT",	ST_VAL,		259,	zo_xop },
{ "ASSIGNED?",	ST_PRED,	255,	zo_xop },
{ "BAND",	ST_VAL,		9,	zo_2op },
{ "BCOM",	ST_VAL,		248,	zo_xop },
{ "BOR",	ST_VAL,		8,	zo_2op },
{ "BTST",	ST_PRED,	7,	zo_2op },
{ "BUFOUT",	0,		242,	zo_xop },
{ "CALL",	ST_VAL|ST_CALL,	224,	zo_call },
{ "CALL1",	ST_VAL|ST_CALL,	136,	zo_1op },
{ "CALL2",	ST_VAL|ST_CALL,	25,	zo_2op },
{ "CATCH",	ST_VAL,		185,	zo_0op },
{ "CLEAR",	0,		237,	zo_xop },
{ "COLOR",	0,		27,	zo_2op },
{ "COPYT",	0,		253,	zo_xop },
{ "CRLF",	0,		187,	zo_0op },
{ "CURGET",	0,		240,	zo_xop },
{ "CURSET",	0,		239,	zo_xop },
{ "DCLEAR",	0,		263,	zo_xop },
{ "DEC",	0,		134,	zo_1op },
{ "DIRIN",	0,		244,	zo_xop },
{ "DIROUT",	0,		243,	zo_xop },
{ "DISPLAY",	0,		261,	zo_xop },
{ "DIV",	ST_VAL,		23,	zo_2op },
{ "DLESS?",	ST_PRED,	4,	zo_2op },
{ "ENDMOVE",	ST_VAL,		271,	zo_xop },
{ "EQUAL?",	ST_PRED|ST_VARARGS,	1,	zo_2op },
{ "ERASE",	0,		238,	zo_xop },
{ "EXTOP",	0,		EXTOP,	zo_0op },
{ "FCLEAR",	0,		12,	zo_2op },
{ "FIRST?",	ST_VAL|ST_PRED,	130,	zo_1op },
{ "FONT",	ST_VAL,		260,	zo_xop },
{ "FSET",	0,		11,	zo_2op },
{ "FSET?",	ST_PRED,	10,	zo_2op },
{ "FSTACK",	ST_YZIP,		277,	zo_xop },
{ "GET",	ST_VAL,		15,	zo_2op },
{ "GETB",	ST_VAL,		16,	zo_2op },
{ "GETP",	ST_VAL,		17,	zo_2op },
{ "GETPT",	ST_VAL,		18,	zo_2op },
{ "GRTR?",	ST_PRED,	3,	zo_2op },
{ "HLIGHT",	0,		241,	zo_xop },
{ "ICALL",	ST_CALL,	249,	zo_icall },
{ "ICALL1",	ST_CALL,	143,	zo_1op },
{ "ICALL2",	ST_CALL,	26,	zo_2op },
{ "IGRTR?",	ST_PRED,	5,	zo_2op },
{ "IN?",	ST_PRED,	6,	zo_2op },
{ "INC",	0,		133,	zo_1op },
{ "INPUT",	ST_VAL,		246,	zo_xop },
{ "INTBL?",	ST_VAL|ST_PRED,	247,	zo_xop },
{ "IRESTORE",	ST_VAL,		266,	zo_xop },
{ "ISAVE",	ST_VAL,		265,	zo_xop },
{ "IXCALL",	0,		250,	zo_xop },
{ "JUMP",	0,		140,	zo_jump },
{ "LESS?",	ST_PRED,	2,	zo_2op },
{ "LEX",	0,		251,	zo_xop },
{ "LOC",	ST_VAL,		131,	zo_1op },
{ "MARGIN",	0,		264,	zo_xop },
{ "MENU",	ST_YZIP|ST_PRED,	283,	zo_xop },
{ "MOD",	ST_VAL,		24,	zo_2op },
{ "MOUSE-INFO",	ST_YZIP,	278,	zo_xop },
{ "MOUSE-LIMIT",	ST_YZIP,	279,	zo_xop },
{ "MOVE",	0,		14,	zo_2op },
{ "MUL",	ST_VAL,		22,	zo_2op },
{ "NEXT?",	ST_VAL|ST_PRED,	129,	zo_1op },
{ "NEXTP",	ST_VAL,		19,	zo_2op },
{ "NOOP",	0,		180,	zo_0op },
{ "ORIGINAL?",	ST_PRED,	191,	zo_0op },
{ "PICINF",	ST_PRED,	262,	zo_xop },
{ "PICSET",	ST_YZIP,	284,	zo_xop },
{ "POP",	0,		233,	zo_xop }, /* YZIP: ST_VAL */
{ "PRINT",	0,		141,	zo_1op },
{ "PRINTB",	0,		135,	zo_1op },
{ "PRINTC",	0,		229,	zo_xop },
{ "PRINTD",	0,		138,	zo_1op },
{ "PRINTF",	ST_YZIP,	282,	zo_xop },
{ "PRINTI",	0,		178,	zo_printi },
{ "PRINTMOVE",	ST_VAL,		267,	zo_xop },
{ "PRINTN",	0,		230,	zo_xop },
{ "PRINTR",	0,		179,	zo_printi },
{ "PRINTT",	0,		254,	zo_xop },
{ "PTSIZE",	ST_VAL,		132,	zo_1op },
{ "PUSH",	0,		232,	zo_xop },
{ "PUT",	0,		225,	zo_xop },
{ "PUTB",	0,		226,	zo_xop },
{ "PUTP",	0,		227,	zo_xop },
{ "QUIT",	0,		186,	zo_0op },
{ "RANDOM",	ST_VAL,		231,	zo_xop },
{ "READ",	ST_VAL,		228,	zo_xop },
{ "REMOVE",	0,		137,	zo_1op },
{ "RESTART",	0,		183,	zo_0op },
{ "RESTORE",	ST_VAL,		257,	zo_xop },
{ "RETURN",	0,		139,	zo_1op },
{ "RFALSE",	0,		177,	zo_0op },
{ "RSTACK",	0,		184,	zo_0op },
{ "RTIME",	ST_VAL,		268,	zo_xop },
{ "RTRUE",	0,		176,	zo_0op },
{ "SAVE",	ST_VAL,		256,	zo_xop },
{ "SCREEN",	0,		235,	zo_xop },
{ "SCROLL",	ST_YZIP,	276,	zo_xop },
{ "SEND",	ST_VAL,		269,	zo_xop },
{ "SERVER",	ST_VAL,		270,	zo_xop },
{ "SET",	0,		13,	zo_2op },
{ "SHIFT",	ST_VAL,		258,	zo_xop },
{ "SOUND",	0,		245,	zo_xop },
{ "SPLIT",	0,		234,	zo_xop },
{ "SUB",	ST_VAL,		21,	zo_2op },
{ "THROW",	0,		28,	zo_2op },
{ "USL",	0,		188,	zo_0op },
{ "VALUE",	ST_VAL,		142,	zo_1op },
{ "VERIFY",	ST_PRED,	189,	zo_0op },
{ "WINATTR",	ST_YZIP,	274,	zo_xop },
{ "WINGET",	ST_YZIP|ST_VAL,	275,	zo_xop },
{ "WINPOS",	ST_YZIP,	272,	zo_xop },
{ "WINPUT",	ST_YZIP,	281,	zo_xop },
{ "WINSIZE",	ST_YZIP,	273,	zo_xop },
{ "XCALL",	ST_VAL,		236,	zo_xop },
{ "XPUSH",	ST_YZIP|ST_PRED,	280,	zo_xop },
{ "ZERO?",	ST_PRED,	128,	zo_1op },
{ "ZWSTR",	0,		252,	zo_xop },
{ NULL,		0,		0,	NULL }
			};
