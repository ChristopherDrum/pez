/* I need unified forward declarations */
/* Its just easier this way */

#include <stdio.h>
#include "zap.h"

#ifndef COMMON_H
#define COMMON_H

/* ALLOC.c */
char *MALLOC(unsigned int);
char *CALLOC(unsigned int, unsigned int);
char *REALLOC(void *, unsigned int);
void FREE(void *);
int CFREE(void *, unsigned int, unsigned int);

/* resfile.c */
void res_map(long , long , long , int );
void res_long(long , int );
void res_short(short , int );
void res_string(char *, int );
void write_a_res(char *, char *, int , int );
void stuff_short(unsigned char *, int , int );
void add_a_resource(char *, int , int , unsigned char *);
char *memq(char *, char *);
void add_vers_resource(int , char *);

/* zap.c */
static int usage( char *, char * );
static void zapinit();
void odinit();
void def_sym( STABLE *, char *, UWORD , int , unsigned char  );
UWORD get_release();
void write_character_counts();
void write_chart();
void stats(int , int );
void print_cpu_time(int );
void print_elapsed_time(int );
void version_update();
void res_out();

/* zasm.c*/
int asline();
int assemble( char * );
int ascomma();
int ascolon();
int assync( int  );
int asnxtoken();

/* comptime.c*/
int print_comptime(char *, FILE *);

/* zobj.c */
int objalign( int );
int objopen( char * );
int objfinish();
int set_serial();
int objclose( int );
int objput( UBYTE *, int );
int objputb( int b );
int objgetb();
int objputw( UWORD w );
int objputzstr( UBYTE * );
int objseek( long , int  );
long objtell();
int objvocab(VNODE *nodes[]);
int objvoc( VNODE * );
static int addvbyte( UBYTE );
static int insvnode( VNODE * );
int voclabel( LGSYMBOL * );
int reccmp(unsigned char *, unsigned char *, int);

/* zapres.c */
unsigned short gen_zpcf_id(char *);
void add_zpcf_resource(char *, char *, int , int , int , int );
void add_options_resource(int , int );

/* zdir.c */
int zd_end( int );
int zd_insert( int dirnum );
int zd_endi( int dirnum );
int zd_false( int dirnum );
int zd_true( int dirnum );
int zd_align( int dirnum );
int zd_byte( int dirnum );
int zd_word( int dirnum );
static int zd_wdbyt(int (*evalrtc)());
int zd_chrset( int dirnum );
int zd_fstr( int dirnum );
int zd_funct( int dirnum );
int zd_gstr( int dirnum );
int zd_gvar( int dirnum );
int zd_len( int dirnum );
int zd_new( int dirnum );
int zd_object( int dirnum );
int zd_page( int dirnum );
int zd_pcset( int dirnum );
int zd_pdef( int dirnum );
int zd_prop( int dirnum );
int zd_segment(int dirnum);
int zd_endseg(int dirnum);
int zd_defseg(int dirnum);
int zd_seq( int dirnum );
int zd_str( int dirnum );
int zd_strl( int dirnum );
static int dostr( int lenF );
int zd_table( int dirnum );
int zd_endt( int dirnum );
static void endtable();
int zd_vocbeg( int dirnum );
int zd_vocend( int dirnum );
int zd_zword( int dirnum );
int zd_options(int dirnum);
int zd_picfile(int dirnum);
/*"unimplemented directives" from here*/
int zd_lang( int dirnum );
int zd_time( int dirnum );
int zd_nyi( int dirnum );
int get_type(BOOL after_comma);

/* zerror.c */
int my_printf(char *msgP, ...);
int my_fprintf(FILE *ff, char *msgP, ...);
int zerror( int when, char *msgP, ... );
void my_exit(int);
int zexpected( int , char * );

/* zeval.c */
int evalbyte();
int evalconst( ZNUM *);
int evalword();
int eval( VALUE *, LGSYMBOL ** );
LGSYMBOL* evalsymref( char * );

/* zfixup.c */
int fixup( int );
static int fix_syms( FIXUP ** );
static int fixupone(FIXUP *);
static int res_fix( FIXUP ** );
int makefixup(LGSYMBOL *, int, long, ZNUM);
int undef();
static int undefsym( LGSYMBOL * );

/* zlabel.c */
int asequate();
int aslabel();

/* zlex.c */
int tklex( LEXBUF **, int *, char * );
int lexdirop(LEXBUF **, int *, char *, FILE *, char *);
int lexlinemark( LEXBUF **, int *, UWORD , FILE * );
int gettoken( FILE *, char **, int );
int lexadd( LEXBUF **, int *, int , char *, int );
char ** litstr( char * );
char *get_buffer();
void free_buffer();
char *expand_buffer();
int buffer_size();

/* zop.c */
int zo_0op( ODDISP *ocP );
int zo_1op( ODDISP *ocP );
int zo_2op( ODDISP *ocP );
int zo_xop( ODDISP *ocP );
int zo_call( ODDISP *ocP );
int zo_icall( ODDISP *ocP );
int zo_gxop( ODDISP *, int );
int zo_jump( ODDISP *ocP );
int zo_printi( ODDISP *ocP );
static int zo_eol( ODDISP *ocP );
static int zo_val( ODDISP *ocP );
static int zo_pred( ODDISP *ocP );
static int zo_putxargs();
static int zo_putargs();
static int zo_getargs( int argmax );

/* zsegment.c */
void add_marker(SEGMENT *, long , int );
void segopen(char *);
void segclose();
void addadj(char *, char *);
void addfun(LGSYMBOL *);
void addstr(LGSYMBOL *);
void addcall(LGSYMBOL *, LGSYMBOL *);
int getnum(char *);
void segstartup(char *);
void addnode(SEGNODE **, SEGMENT *, SEGMENT *);
void additem(ITEM **, LGSYMBOL *);
void segout(char *);
void putlist(ITEM *);
void segput(char *);
void segputl(char *, int );
char *itoa(long);
void segfile(char *);
void addpages(UWORD , UWORD , char[]);
void setpages(SEGMENT *);
void funpages(ITEM *, SEGMENT *);
char *setadj(SEGNODE *);
char **segstr( char * );

/* zstring.c */
int addfstr( char * );
UBYTE *translate_char(UBYTE *, UBYTE *);
UBYTE *zstr( UBYTE *, int , int  );
int zstrlen( UBYTE * );
void zcset( UBYTE * );
void startzstr( UBYTE *, int  );
void addzchar( UBYTE ch );
void endzstr();
int zchar_size(UBYTE , int *);
int fw_get_hash(UBYTE *, int );
void fw_string(UBYTE *);
void dump_frequent_words(char *);
void sort_stuff(int);
void print_stuff(char *, int );
void fw_set_worst(int ct);
void fw_dealloc(int at);

/* zsymout.c */
void init_sym_stuff(char *);
void finish_up_syms(char *);
ST_SYMTAB *sym_dump(STABLE *, long , int );
int process_symbol(SYMBOL *);
int is_addr_type(int);
void process_add_info(LGSYMBOL *);
SF_PTR process_sym_name(char *);
void make_new_links(ST_D_LNK **, ST_D_LNK **, char *, long, SF_PTR);
ST_D_LNK **to_array(ST_D_LNK *, unsigned int);
ST_D_LNK **sort_by_name(ST_D_LNK *,unsigned int );
ST_D_LNK **sort_by_val(ST_D_LNK *, unsigned int );
int sym_open(char *);
void st_output_compat_info(char *);
void do_sym_fix(ST_SYMTAB *, SF_PTR );
void sym_close();
void st_output_thing(char *, int );
int st_output_arr(ST_D_LNK **, unsigned int , int );
void st_move_to(SF_PTR);
void st_output_skip(int);
void start_links();
void init_links();
ST_D_LNK *get_link();


/* zsymtab.c */
STABLE *symnew(int , int );
void symempty(STABLE *, int );
SYMBOL *symenter(STABLE *, char *, int );
SYMBOL *symlookup(STABLE *, char *);
int symwalk(STABLE *, int (*rtc)());
int hash(char *, int );

/* ztable.c */
void set_table(TABLE_INFO *);
TABLE_INFO *new_table(unsigned char , UBYTE );
TABLE_INFO *array_table();
void set_els(unsigned char, unsigned char, unsigned short);
void set_up_vocab(unsigned char);

#endif