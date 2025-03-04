/*	zap.h		General definitions for ZAP

	March 1988 by Zinn Computer Company

*/

#ifndef	H_ZAP				/* Allow multiple inclusions */
#define	H_ZAP

#ifndef	TRUE				/* Define some standard things */
#define	TRUE	1
#define	FALSE	0
#endif	TRUE

#ifndef	NULL
#define	NULL	((char *)0)
#endif	NULL

#ifndef	NUL
#define	NUL	'\0'
#endif	NUL


	/* Portability - adjust these to fit the machine */

typedef	char		BOOL;		/* Best TRUE/FALSE type */
typedef	char		BYTE;		/* Very small number */
typedef	unsigned char	UBYTE;		/* Unsigned 8 bit */
typedef short		WORD;		/* 16 bit */
typedef	unsigned short	UWORD;		/* 16 bit unsigned */
typedef	long		ZNUM;		/* Size of number handled by eval */


	/* Environmental constants */

#define	BIAS2OP		192		/* Make 2-op into ext-op */
#define	EXTOP		190		/* Extended opcode prefix */
#define	FUNARGS		16		/* Max # of function args */
#define	GSHASHSIZE	5003		/* Global symbol table hash size */
#define	GVARBASE	16		/* First global variable # */
#define	GVARMAX		255		/* Maximum global variable # */
#define	LSHASHSIZE	509		/* Local symbol table hash size */
#define	LTHASHSIZE	5003		/* Literal string table hash size */
#define STHASHSIZE      23              /* Segment name table hash size */
#define	LXBSIZE		16000		/* Lex/token buffer size */
#define	MAXFILES	1		/* Max # files on command line */
#define	STRINGBUF	512		/* Starting size of string buffers */
#define	MAXFREQSTR	96		/* Maximum # of frequent strings */
#define	OBBSIZE		16000		/* Object buffer size */
#define	ODHASHSIZE	163		/* Opcode/directive table hash size */
#define XZIP		5		/* XZIP .new version number */
#define YZIP		6		/* YZIP .new version number */
#define	VERS		5		/* Default version */
#define OBJ_LEN_LOC	26		/* Where length info goes in file */
#define OBJ_CHK_LOC	28		/* Where checksum info goes in file */
#define OBJ_SERIAL_LOC  18		/* Where "serial number" goes */
#define OBJ_SERIAL_LEN  6
#define OBJ_ENDLOD_LOC  4
#define REL_LOC		2		/* Where release is stored in zip file */
#define NUM_PAGES       1280            /* Number of possible pages in object file */
#define PAGE_SIZE       512
#define ENDLODSYM       "ENDLOD"        /* End of preload tables sym name */
	/* Register assignment optimizations */

#define	AREG1		register
#define	AREG2		register
#define	AREG3           register
#define	AREG4
#define	AREG5
#define	AREG6
#define	AREG7
#define	DREG1		register
#define	DREG2		register
#define	DREG3           register
#define	DREG4
#define	DREG5
#define	DREG6
#define	DREG7

	/* Status codes */

#define	SCOK		0		/* Good.  Otta be zero. */
#define	SCERR		1		/* General error */
#define	SCCMDLINE	2		/* Command line error */
#define	SCNOFILE	3		/* No file */
#define	SCEOF		4		/* End of file */
#define	SCMEMORY	5		/* Out of memory */
#define	SCEND		6		/* End of source (fake status) */
#define	SCNOTDONE	7		/* Something not done */

	/* Token types */
#define	TKOTHER		0		/* Some char sequence ... dunno what */
#define	TKSYMBOL	1		/* A symbol-type string */
#define	TKFILE		2		/* A source file */
#define	TKDIRECTIVE	3		/* Assembler directive */
#define	TKOPCODE	4		/* ZAP opcode */
#define	TKLABEL		5		/* Simple label */
#define	TKGLABEL	6		/* Global label */
#define	TKEQUATE	7		/* Equate label */
#define	TKSTRING	8		/* Quoted string */
#define	TKINTEGER	9		/* Some integer */
#define	TKCHAR		10		/* Character */
#define	TKLINE		11		/* Line mark */
#define	TKEOL		12		/* End of line */
#define	TKEOB		13		/* End of token buffer */
#define	TKEOF		14		/* End of file */

	/* Classes for fixup of forward references */

#define	FXBYTE		0		/* Byte */
#define	FXWORD		1		/* Word */
#define	FXVAR		2		/* Global variable */
#define	FXFPREDS	3		/* False Predicate, short */
#define	FXFPREDL	4		/* False Predicate, long */
#define	FXTPREDS	5		/* True Predicate, short */
#define	FXTPREDL	6		/* True Predicate, long */
#define	FXJUMP		7		/* Jump... */

	/* Assembler states */

#define	AS_FUNCT	0x01		/* Within a function */
#define	AS_TABLE	0x02		/* Within a table */
#define	AS_VOCAB	0x04		/* Within vocabulary area */

	/* Error time masks */
#define	E_PASS1		0x01		/* Pass1 */
#define	E_PASS2		0x02		/* Pass2 */
#define	E_ALWAYS	0x03		/* All times */

	/* Flag bits for symbol tables */

#define	STB_COPYNAME	0x01		/* Must make copy of symbol/name */


	/* Flag bits for symbols */

#define	ST_GLOBAL	0x01		/* Global symbol (vs local) */
#define	ST_DEFINED	0x02		/* Has been defined */
#define	ST_REL		0x04		/* Relocatable (vs constant) value */
#define	ST_VAR		0x08		/* Variable */
#define ST_KILL		0x10		/* Half-kill */

	/* Bits for symbols in the opcode/directive symbol table */

#define	ST_DIRECTIVE	0x01		/* Directive (vs opcode) */
#define ST_YZIP		0x10		/* new to YZIP ? */
#define ST_CALL         0x20            /* Is it a call; used for segment file */
#define ST_CSTR         0x40            /* Does it print a constant string; " */

  /* Bits applicable to directives only */
#define	ST_INSERT	0x02		/* Insert another file */
#define	ST_ENDI		0x04		/* End of insertion */

  /* Bits applicable to opcodes only */
#define	ST_VAL		0x02		/* Returns a value */
#define	ST_PRED		0x04		/* Predicate opcode */
#define	ST_VARARGS	0x08		/* Takes 2 or more args ("equal?") */

/* The following defines are for symbol table generation */

#define ST_VERSION 2
#define SPACESUSED 14		/* Number of spaces used by assembler */

#define DEC_POOLS 100		/* These used in alloc. of dec_links */
#define DEC_POOL_SIZE 500

#define NAME_HASH_SIZE 509

#define SF_PTR long

#define COMP_START 9
#define COMP_LENGTH 6
#define COMP_LOC 2
#define HEAD_ST_LOC COMP_LOC + sizeof(SF_PTR)
#define BOUNDS_TABLE_LOC (HEAD_ST_LOC + sizeof(ST_SYMTAB))

#define SBSIZE 131072
#define SBNUM  128

#define HEADER_LEN 16

#define MAX_LABELS 100

#define ZWORD_SIZE 6
#define OBJECT_SIZE 14
#define	SSHASHSIZE	23		/* Structure symbol table hash size */

#define NSEGS 500
#define SEGBNUM 128
#define SEGBSIZ 131072

/* These are the various space types */

#define VARIABLE_SPACE	0
#define LABEL_SPACE		1
#define BOUNDS_TABLE_LOW LABEL_SPACE
#define TABLE_SPACE		2
#define FUNCTION_SPACE	3
#define ZSTRING_SPACE	4
#define FSTRING_SPACE	5
#define BOUNDS_TABLE_HIGH FSTRING_SPACE
#define OBJECT_SPACE	6
#define PROPERTY_SPACE	7
#define FLAG_SPACE		8
#define VERB_SPACE		9
#define PICTURE_SPACE	10
#define SOUND_SPACE		11
#define CONSTANT_SPACE	12
#define SPACE_SPACE		13
#define BOOLEAN_SPACE	247
#define CHARACTER_SPACE	248
#define NUMBER_SPACE	253
#define ANY_SPACE		254

/* Here are the various data types. */

#define UNKNOWN_TYPE	0
#define BYTE_TYPE		1
#define WORD_TYPE		2
#define ARRAY_TYPE		3
#define STRUCTURE_TYPE	4
#define ZSTRING_TYPE	5
#define OBJECT_TYPE		6
#define PTABLE_TYPE		7
#define VWORD_TYPE		8

/* Table statuses */

#define NOT_IN_TABLE	0
#define AT_TABLE_HEADER	1
#define IN_TABLE		2
#define VOCAB_TABLE		3

/*
 
*//*	Structures */


/* symbol structure (used as a prefix to the symbol data structure) */
typedef
  struct symbol {
    struct symbol	*sym_next;	/* link to next symbol in same bucket */
    char		*sym_name;	/* symbol name */
  } SYMBOL;

typedef					/* Symbol table structure */
  struct stable {
    SYMBOL		**stab_buckets;	/* hash buckets */
    int			stab_hsize;
    UWORD		stab_flags;
  } STABLE;


/* The following structures are used for segment file generation. */
struct lg_symbol;

typedef
  struct item {
    struct item *next;
    struct lg_symbol *sym;
  } ITEM;

typedef struct sm {
  struct sm *next;
  long beginning_pc;
  long ending_pc;} segment_marker;

typedef
  struct segment {
    char *name;
    ITEM *functions;
    ITEM *strings;
    segment_marker *segs;	/* Tables in this segment */
    char pages[NUM_PAGES / 8];
  } SEGMENT;

typedef
  struct segnode {
    SEGMENT *seg1;
    SEGMENT *seg2;
    struct segnode *next;
  } SEGNODE;


/* The following structures are for symbol table generation */
typedef
	struct st_table {				/* An array of symbols */
		unsigned short	length;	
		SF_PTR			decsptr;	/* Pointer into syms file */
	} ST_TABLE;

typedef
  struct st_symtab {					/* A symbol table header */
    unsigned short	num_of_spaces;  /* Number of spaces used */
    SF_PTR			n_tableptr;		/* Ptr to name table */
    SF_PTR			s_tablesptr;	/* Ptr to array of space tables */
  } ST_SYMTAB;


typedef
  struct st_d_lnk {			/* Used to make a linked list of decs */
    SF_PTR			decptr; /* Declaration's location in file */
    union {
      long value;			/* Declaration's value */
      char *name;			/* Its name */
    } 				sortval;
    struct st_d_lnk	*next;
	} ST_D_LNK;

typedef
	struct str_fixup_node {
		SF_PTR					header_loc;
		ST_SYMTAB				*header;
		STABLE					*syms;
		struct str_fixup_node	*next;
	} STR_FIXUP_NODE;

typedef					/* General value passed around */
  struct {
    UWORD		v_flags;	/* Flags... */
    ZNUM		v_value;	/* Value */
  } VALUE;


typedef					/* Symbol in opcode/directive tbl */
  struct {
    SYMBOL		od_sym;		/* Common symbol prefix */
    VALUE		od_val;		/* Symbol value */
    } ODSYMBOL;

/* The following go into LGSYMBOLs */

typedef
	struct table_info {
		unsigned char type;
		union {
			struct {
				unsigned char 		val_space;
			} word_or_byte;
			struct {
				unsigned short		el_size;
				unsigned short		num_els;
				struct table_info 	*el_type;
			} array;
			struct {
				STABLE				*el_table;
			} structure;
		} it;
	} TABLE_INFO;

typedef 		/* This is added to the LGSYMBOL structure */
	union lg_add_info {
		struct {
			unsigned char 	min_args;
			unsigned char 	max_args;
			unsigned char 	value_space;
			unsigned char	_unused;
			ST_SYMTAB	*syms;
			ITEM            *calls;
			union {
			  struct lg_symbol *lastcall;
			  SEGMENT          *lastseg;
			} temp;
			UWORD           start_page;
			UWORD           end_page;
		} function;
		struct {
		        UWORD           start_page;
			UWORD           end_page;
			struct lg_symbol *last_use;
	        } string;
		struct {
			unsigned char 	space;
		} variable;
		TABLE_INFO *table;
	} LG_ADD_INFO;
	
typedef					/* Symbol in local/global tables */
  struct lg_symbol {
    SYMBOL		lg_sym;		/* Common symbol prefix */
    VALUE		lg_val;		/* Symbol value */
    long		lg_loc;		/* Location at which defined */
    unsigned char	lg_space;	/* Symbol space */
    unsigned char	lg_referenced; /* non-zero if symbol ever referenced */
    LG_ADD_INFO	        lg_add_info;
    } LGSYMBOL;

typedef					/* Forward reference fixup block */
  struct fixup {
    struct fixup	*fx_next;	/* Link to next fixup block */
    LGSYMBOL            *fx_sym;
    UBYTE		fx_class;	/* Fixup class */
    long		fx_addr;	/* Fixup address */
    long                fx_offset;
  } FIXUP;


typedef					/* Block for lexicalized input */
  struct lexbuf {
    struct lexbuf	*lx_link;	/* Link to next block */
    UBYTE		lx_data[LXBSIZE]; /* The stream of lexed tokens */
    } LEXBUF;


typedef					/* Opcode/directive definition */
  struct {
    char		*od_name;	/* Name of opcode/directive */
    int			od_flags;	/* Flags attached */
    int			od_value;	/* Some value */
    int			(*od_disp)();	/* Routine that handles it */
    } ODDISP;


typedef					/* For a label in a vocab section */
  struct vlabel {
    struct vlabel	*vl_nextP;	/* Next label (if any) */
    LGSYMBOL		*vl_symP;	/* Ptr to label block */
    UWORD		vl_offset;	/* Label offset in vnode */
    } VLABEL;


typedef					/* Vocabulary record tree node */
  struct vnode {
    struct vnode	*vn_leftP;	/* Left node */
    struct vnode	*vn_rightP;	/* Right node */
    struct vlabel	*vn_labelP;	/* Label info */
    FIXUP *vn_fixups;		/* Chain of fixups for this word */
    UBYTE		vn_data[1];	/* Data */
    } VNODE;

/*
 
*//*  misc stuff */

	/* Useful macros */

/* anyarg() - is any argument next in token stream? */
#define	anyarg()	( ( *LextkP == TKINTEGER ) || (*LextkP == TKSYMBOL ) )

#endif	H_ZAP
