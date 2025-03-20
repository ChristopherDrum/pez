/*	zap.c		Main module of ZAP assembler

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	main		The main routine

Plus internal routines:

	usage		Print a program usage message
	version_update	Update Zap parameters
*/

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>

#include "zap.h"


/* Local definitions */


/* External routines */

extern	SYMBOL 	*symenter();
extern	SYMBOL	*symlookup();
extern	STABLE	*symnew();
extern	ST_SYMTAB	*sym_dump();
extern char     **litstr();
extern int my_printf();

/* External data */

extern	int	build_freq;
extern  FILE	*Error_out;
extern	ODDISP	Dirtbl[];
extern	STABLE	*Gblsymtab;
extern	int	GvarC;
extern  int	Objtop;
extern	int	ObjectC;
extern	int	VocC;
extern	STABLE	*Lclsymtab;
extern	STABLE	*Litsymtab;
extern  STABLE  *Segsymtab;
extern	ODDISP	Octbl[];
extern	STABLE	*Odsymtab;
extern	BOOL	Opsrcerr;
extern	int	Pass;
extern	BOOL	Permshift;
extern	UWORD	State;
extern	long	S_lexmax;
extern	long	S_lexsize;
extern	long	S_lexttl;
extern	long	S_objdata;
extern	long	S_objsize;
extern	long	S_srcsize;
extern	char	Zcsnorm[];
extern	ST_SYMTAB	*glbl_tab;
extern	int	Version;
extern	UWORD	Release;
extern  BOOL    yoffsets;
extern  int	character_count;  /* if one, print out count of characters used */
extern  long    character_counts[];

/* Local routines (and forward references) */
UWORD	get_release();

/* Local data */

BOOL    xoption = FALSE;        /* Suppress YZIP offset format */

/* Private data */

static	char	*Files[MAXFILES];
static	char	*Objfile;		/* Ptr to object filename */
static	char	Objfnm[129];		/* Object filename, maybe */
static char Error_file[128];	/* foo.errors */
static char Chart_file[128];
static char Game_name[128];	/* Just the name of the game */
static char Sym_name[128];
static char Tobjfnm[128];
static char Tsymfnm[128];

/*
 
*//* main( argc, argv, envp )

	The main routine for the ZAP assembler

Accepts :

	< normal entry arguments >

Returns :

	<value>		Status code.  0 is "good".

*/


int main( argc, argv, envp )
int argc;		/* Argument count */
char **argv;		/* Argument vector */
char **envp;		/* Environment vector */
{
  int             i;              /* Scratch */
  int  j;
  int             n;              /* Scratch again */
  int             c;              /* A character */
  int		sts;
  char            *aptr;          /* Argument pointer */
  int             nf;             /* number of files on command line */
  struct tms timebuf;
  int cpu_time, starting_time = (int)time(0);
  print_comptime("zap", 0);
  nf = 0;				/* No files on command line */

  /* Collect command line arguments before doing anything */
  for( i = 1; i < argc; ++i ) {
    /* Check for command line option.  We'll allow the unix
       style "-x" form as well as the original ZAP "/x" form. */
    if ( ( argv[i][0] == '-' ) || ( argv[i][0] == '/' ) ) {
      c = argv[i][1];
      if ( isupper(c) )		/* Allow either-case */
	c = tolower( c );
      n = i;			/* Assume value is bound to option */
      aptr = NULL;		/* Now verify that */
      if ( argv[i][2] != NUL ) {
	if ( argv[i][2] == '=' ) {
	  if ( argv[i][3] != NUL )
	    aptr = &argv[i][3]; }
	else			/* No "=" after option char */
	  aptr = &argv[i][2]; }
      if ( aptr == NULL ) {	/* If no argument with option */
	if ( i < argc-1 ) {	/*  use next token as arg, if any */
	  ++n;
	  aptr = argv[n]; } }
      /* We're finally ready to process the option char */
      switch ( c ) {
      case 'a':		/* Toggle "Print source lines
			   along with errors" flag */
	Opsrcerr = !Opsrcerr;
	break;
	
      case 'o':		/* Output file */
	if ( Objfile != NULL )
	  return( usage( "Duplicate object file given.\n",
			NULL ) );
	Objfile = aptr;
	i = n;
	break;

      case 'c':			/* Count chars */
	character_count = !character_count;
	for (j = 0; j < 256; j++)
	  character_counts[j] = 0;
	break;
      case 'f':			/* Set flag saying build freq words table */
	build_freq = 1;
	printf("Building frequent words table.\n");
	break;

      case 'p':		/* Toggle permanent-shift flag,
			   which indicates whether such
			   things are allowed in Z-strings.  */
	Permshift = !Permshift;
	break;
			
      case 'r':		/* Set release number */
	if (aptr != NULL) {
	  if (atoi(aptr) <= 0) 
	    return(usage("Bad release number given",NULL));
	  else
	    Release = atoi(aptr); }
	else
	  return(usage("Release number not given",NULL));
	i = n;
	break;

      case 'x':		/* Toggle YZIP offset suppression */
	xoption = !xoption;
	break;
		    
      default:		/* Dunno what it is! */
	fprintf( stderr, "Ignoring unknown option %s\n", 
		argv[i] );
	break; } }
    else {				/* Not an option */
      if ( nf == MAXFILES )	/* See if we can take a file */
	return ( usage( "Too many file names", argv[i] ) );
      Files[nf++] = argv[i];	/* Remember the filename */ } }

    /* Command line is in. */
  if ( nf == 0 )			/* No file given ? */
    /* It ought to be OK to assemble from stdin, but nobody's
       asked us to do that, so we'll defer it for now. */
    return( usage( "No file to assemble!", NULL ) );

  if ( Objfile == NULL ) {		/* No object file given? */
    /* Must determine object filename from [first] input filename */
    Objfile = &Objfnm[0];
    strcpy( &Objfnm[0], Files[0] );
    if ( ( i = strlen( &Objfnm[0] ) ) > 4 )
      /* Check for ".zap" or ".ZAP" on the end */
      if ( ( strcmp( &Objfnm[i-4], ".zap" ) == 0 ) ||
	  ( strcmp( &Objfnm[i-4], ".zap" ) == 0 ) )
	Objfnm[i-4] = NUL;	/* Remove the .zap */
    strcpy(&Game_name[0], &Objfnm[0]);
    strcat( &Objfnm[0], ".zip" );	/* Add the zip */
  }
  strcpy(&Error_file[0], &Game_name[0]);
  strcat(&Error_file[0], ".errors");
  strcpy(&Chart_file[0], &Game_name[0]);
  strcat(&Chart_file[0], ".chart");
  Error_out = fopen(&Error_file[0], "w");
  print_comptime("zap", Error_out);
 
  strcpy(&Tobjfnm[0], &Game_name[0]);
  strcat(&Tobjfnm[0], ".tzp");
  strcpy(&Tsymfnm[0], &Game_name[0]);
  strcat(&Tsymfnm[0], ".tsy");
  strcpy(&Sym_name[0], &Game_name[0]);
  strcat(&Sym_name[0], ".syms");

  /* Initialize tables, files, etc. */
  init_sym_stuff(&Tsymfnm[0]); /* Initializes symbol table info */
  zapinit();				/* Initialize general stuff */
  odinit();				/* Init opcode/directive tables */
  if (!build_freq) {
    if ( ( sts = objopen( &Tobjfnm[0] ) != SCOK ) )
      return( sts );	}		/* Error opening object */
  
  /* Do the assembly */
  assemble( Files[0] );
  
  if (!build_freq) {
    /* Check out undefined symbol refs */
    undef();
  
    /* Finish the object file */
    if ((sts = objfinish()) != SCOK)
      return (sts);
    write_chart();		/* Chart file */
    if (character_count)
      write_character_counts();
    objclose(TRUE);
    if (rename(&Tobjfnm[0], Objfile) < 0)
      perror("Rename of object file failed");
  
    /* Output global symbol table */
    glbl_tab = sym_dump(Gblsymtab,0,1);
    res_out();			/* Write res files for syms and zip */

    /* Finish up symbol table stuff */
    finish_up_syms(Objfile);
    if (rename(&Tsymfnm[0], &Sym_name[0]) < 0)
      perror("Rename of .syms file failed");

    if (yoffsets)
      segout(&Game_name[0]);  /* Output segment file */ }
  else {
    dump_frequent_words(&Game_name[0]); }

  /* Print statistics */
  times(&timebuf);
  cpu_time = (int)timebuf.tms_utime + (int)timebuf.tms_stime;
  stats(cpu_time, starting_time);

  /* Clean up tables, files, etc. */
}
/*
 
*//* usage( msgP, tokenP )

	Print the invocation usage and optional helpful message

Accepts :

	msgP		Ptr to message to give, or NULL if no message.
	tokenP		Ptr to offending token, or NULL if none.

Returns :

	<value>		SCCMDLINE, so that caller can exit( usage() );

*/

static int
usage( msgP, tokenP )
	char		*msgP;
	char		*tokenP;
{
    fprintf( stderr, "usage: zap source-file -o object-file\n" );
    if ( tokenP != NULL )
	fprintf( stderr, "Error at %s%s", tokenP, msgP==NULL?".\n":" -- " );
    if ( msgP != NULL )
	fprintf( stderr, "%s\n", msgP );
    return( SCCMDLINE );
}
/*
 
*//* zapinit()

	General initialization of things

*/

static
zapinit()
{
    /* Allocate symbol tables */
    Gblsymtab = symnew( GSHASHSIZE, 0 );
    Lclsymtab = symnew( LSHASHSIZE, 0 );
    Litsymtab = symnew( LTHASHSIZE, STB_COPYNAME );
    Segsymtab = symnew( STHASHSIZE, STB_COPYNAME );
	
	if (Release == 0)
		Release = get_release();
    my_printf("Release: %d\n", Release);

    /* Define standard variables */
    def_sym( Gblsymtab, "STACK", ST_VAR, 0, VARIABLE_SPACE );
    def_sym( Gblsymtab, "ZORKID", 0, Release, CONSTANT_SPACE);

    /* Init global variables and things */
    State = 0;
    Pass = 0;
    GvarC = GVARBASE;

    /* Specify standard character set */
    zcset( &Zcsnorm[0] );
}
/*
 
*//* odinit()

	Initialize opcode/directive data

*/

odinit()
{
DREG1	int		odnum;		/* O/d number */
AREG1	ODDISP		*odP;		/* Ptr to a table */
AREG2	ODSYMBOL	*odsymP;	/* Ptr to a symbol */

    /* Create a symbol table for opcodes and directives */
    Odsymtab = symnew( ODHASHSIZE, 0 );

    /* Init the directives */
    for( odP = &Dirtbl[0], odnum = 0; odP->od_name != NULL; ++odP ) {

	/* Enter this name */
	odsymP = (ODSYMBOL *)symenter( Odsymtab,
				 odP->od_name, sizeof(ODSYMBOL) );

	/* Form the entry.  Value is dispatch table index */
	odsymP->od_val.v_flags = ST_DIRECTIVE | odP->od_flags;
	odsymP->od_val.v_value = odnum++;
    }

    /* Init the opcodes */
    for( odP = &Octbl[0], odnum = 0; odP->od_name != NULL; ++odP ) {

	/* Enter this name */
	odsymP = (ODSYMBOL *)symenter( Odsymtab,
				 odP->od_name, sizeof(ODSYMBOL) );

	/* Form the entry.  Value is dispatch table index */
	odsymP->od_val.v_flags = odP->od_flags;
	odsymP->od_val.v_value = odnum++;
    }
}
/*
 
*//* def_sym( symtabP, nameP, flags, value )

	Define a symbol at initialization time

Accepts :

	symtabP		Ptr to the symbol table
	nameP		Name of symbol to define
	flags		Flags for symbol
	value		Value for symbol

Returns :

	<nothing>

*/

def_sym( symtabP, nameP, flags, value, space )
	STABLE		*symtabP;	/* Symbol table header */
	char		*nameP;		/* Symbol name */
	UWORD		flags;		/* Symbol flags */
	int		value;		/* Symbol value */
	unsigned char	space; /* Symbol space */
{
AREG1	LGSYMBOL	*symP;		/* Ptr to a symbol */

    /* Enter the symbol in the table */
    symP = (LGSYMBOL *)symenter( symtabP, *(litstr(nameP)), sizeof(LGSYMBOL) );

    /* Form the entry from the input values */
    symP->lg_val.v_flags = ST_DEFINED|flags;
    symP->lg_val.v_value = value;
	symP->lg_space = space;
}
/*
 
*//* get_release()

	This procedure looks at the last release of game.zip and finds the
	release number, adding one.  If there is no previous release, this
	release becomes release 1.
	
*/

UWORD
get_release()
{
	int fp;
	UWORD rel;
	
	if ((fp = open(Objfile,O_RDONLY)) < 0)
		rel = 0;
	else {
		if (lseek(fp, REL_LOC,0) < 0)
		  rel = 0;
		else if (read(fp, &rel, 2) != 2)
		  rel = 0;
		close(fp);
	}
	return(++rel);
}

write_character_counts()
{
  unsigned char chrs[256];
  int i, j;
  long tmp;
  for (i = 0; i < 256; i++)
    chrs[i] = i;
  /* First sort by frequency */
  for (i = 0; i < 255; i++) {
    for (j = 0; j < 255 - i; j++) {
      if (character_counts[j] < character_counts[j + 1]) {
	tmp = character_counts[j];
	character_counts[j] = character_counts[j + 1];
	character_counts[j + 1] = tmp;
	tmp = chrs[j];
	chrs[j] = chrs[j + 1];
	chrs[j + 1] = tmp; } } }
  my_printf("Char  Frequency\n");
  for (i = 0; i < 256; i++) {
    if (character_counts[i] == 0) break;
    if (chrs[i] == 0)
      my_printf ("  ^@");
    else if (chrs[i] < ' ')
      my_printf("  ^%c", chrs[i] + 'A');
    else if (chrs[i] == ' ')
      my_printf("  SP");
    else if (chrs[i] == 127)
      my_printf("  ^?");
    else if (chrs[i] > 127)
      my_printf(" %3o", chrs[i]);
    else
      my_printf("   %c", chrs[i]);
    my_printf("%11d\n", character_counts[i]); } }

/* write_chart()

   Write game.chart file, with line for each assembly */

int endlod = 0;

write_chart()
{
  int out, ct;
  char chart_buf[255];
  struct tm *tms;
  long tt = time(0);
  objseek(OBJ_ENDLOD_LOC, FALSE);
  endlod = objgetb() << 8;
  endlod |= objgetb();
  out = open(&Chart_file[0], O_APPEND | O_WRONLY);
  if (out < 0) {
    out = open(&Chart_file[0], O_CREAT | O_WRONLY, 0666);
    sprintf(&chart_buf[0], "-date-  -rel-  -size-   -pre-  -obj-  -glo-  -voc-\n");
    write(out, &chart_buf[0], strlen(&chart_buf[0])); }
  tms = localtime(&tt);
  sprintf(&chart_buf[0], " %2d/%02d   %3d   %6d   %5d  %5d   %3d   %5d\n",
	  tms->tm_mon+1, tms->tm_mday, Release, Objtop, endlod,
	  ObjectC, GvarC - GVARBASE, VocC);
  write(out, &chart_buf[0], strlen(&chart_buf[0]));
  close(out);
  sprintf(&chart_buf[0], "from %s.zip, %2d/%02d/%02d %d:%02d:%02d.",
	  Game_name, tms->tm_mon+1, tms->tm_mday, tms->tm_year,
	  tms->tm_hour, tms->tm_min, tms->tm_sec);
  add_vers_resource(Release, &chart_buf[0]); }

/*
 
*//* stats(cpu_time, starting_time)

	Print summary of assembly

Accepts :

Returns :

*/

stats(cpu_time, starting_time)
int cpu_time, starting_time;
{
    my_printf( "\n" );
    my_printf( "Lexical buffer space allocated:  %ld\n", S_lexttl );
    my_printf( "Max lexical space allocated at any one time: %ld\n", S_lexmax );
    my_printf( "Object buffer space allocated:  %ld\n", S_objsize );
    my_printf( "Source bytes read:  %ld\n", S_srcsize );
    my_printf( "Object bytes written:  %ld\n", S_objdata );
    my_printf("Preload: %d; %d objects; %d globals; %d words.\n",
	      endlod, ObjectC, GvarC - GVARBASE, VocC);
    print_cpu_time(cpu_time);
    print_elapsed_time(starting_time);
}

#define hour_fact (60 * 60 * 60)
#define min_fact (60 * 60)
#define sec_fact 60

print_cpu_time(cpu_time)
int cpu_time;
{
  int any = 0;
  int h, m, s;
  h = cpu_time / hour_fact;
  cpu_time = cpu_time % hour_fact;
  m = cpu_time / min_fact;
  cpu_time = cpu_time % min_fact;
  s = cpu_time / sec_fact;
  my_printf("Used ");
  if (h > 0) {
    my_printf("%d:", h);
    any = 1; }
  if (any)
    my_printf("%02d:", m);
  else if (m > 0) {
    my_printf("%d:", m);
    any = 1; }
  if (any)
    my_printf("%02d\n", s);
  else
    my_printf("%d\n", s); }

print_elapsed_time(starting_time)
int starting_time;
{
  int elapsed, any = 0, h, m, s;
  elapsed = (int)time(0) - starting_time;
  h = elapsed / (60 * 60);
  elapsed = elapsed % (60 * 60);
  m = elapsed / 60;
  s = elapsed % 60;
  my_printf("Elapsed time ");
  if (h > 0) {
    my_printf("%d:", h);
    any = 1; }
  if (any)
    my_printf("%02d:", m);
  else if (m > 0) {
    my_printf("%d:", m);
    any = 1; }
  if (any)
    my_printf("%02d\n", s);
  else
    my_printf("%d\n", s); }

/*
 
*//* version_update()
	This procedure updates various zap parameters according to the version
	of zip being assembled. */
	
version_update()
{
	ODSYMBOL *sym;
	
	switch(Version) {
	case YZIP:
		/* Now pop returns a value */
		if ((sym = (ODSYMBOL *)symlookup(Odsymtab, "POP")) == NULL)
			zerror(E_PASS1,"Oops! 'Pop' not in opcode/directive table!");
		else {
			sym->od_val.v_flags |= ST_VAL;
			Octbl[sym->od_val.v_value].od_flags |= ST_VAL;
		}
		if (!xoption)
		  yoffsets = TRUE;  /* Turn on YZIP offsets */
		break;
	default:
		break;
	}
}

res_out()
{
  char fname[255];
  char *slash;
  int len, fd;
  strcpy(&fname[0], &Game_name[0]);
  if (slash = strrchr(&fname[0], '/'))
    slash = &slash[1];
  else
    slash = &fname[0];
  len = strlen(slash) + 1;
  while (len--)
    slash[len + 1] = slash[len];
  slash[0] = '%';
  len = strlen(&fname[0]);
  strcat(&fname[0], ".zip");
  write_a_res(&fname[0], "ZdbG", 1, 1);
  fname[len] = 0;
  strcat(&fname[0], ".syms");
  write_a_res(&fname[0], "ZdbT", 1, 0);
}
