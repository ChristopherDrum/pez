/* zsymout.c	Make a symbol table file "game.SYMS"

	Written by Matt Hillman
	
*/

#define __SEG__ Seg2

#include <stdio.h>
#include <fcntl.h>

#include "zap.h"

extern char *MALLOC();
extern char *CALLOC();
extern char *REALLOC();

/* int printsym(); */
ST_SYMTAB *sym_dump();

extern int build_freq;
extern LGSYMBOL **label_list;
extern ST_SYMTAB *glbl_tab;
extern BOOL yoffsets;
extern long funct_start;
extern long string_start;

/* Private global variables for syms file generation. */

static int symfd;	/* File descriptor for symbol file */
static SF_PTR symfloc;	/* The current file pointer location */

static ST_D_LNK *name_list;
static ST_D_LNK *table_list;	/* For bounds-checking table */
static ST_D_LNK *space_lists[SPACESUSED];
static unsigned int name_num;
static unsigned int table_num;
static unsigned int space_nums[SPACESUSED];
int links_init = 0;

static long stoffset;

static STR_FIXUP_NODE *str_list;


/* debugging routines */
/*
bugprint(m)
	char *m;
{
	int i;

	my_fprintf(stderr,"%s...\n",m);
	for (i = 0;i < 10; i++);
}


yawn()
{
	int i;
	
	my_fprintf(stderr,"yawn...\n");
	for (i = 0; i < 100000; i++);
	my_fprintf(stderr,"ahhhh.\n");
}


printsym(sym)
	SYMBOL *sym;
{
	my_fprintf (stderr,"%s\n",sym->sym_name);
	return(SCOK);
}

*/


/***
	init_sym_stuff
		This procedure calls the other various initialization routines.
***/

init_sym_stuff(fname)
	char *fname;
{
	int i;
	unsigned short ver = ST_VERSION;
	if (!build_freq) {
	  sym_open(fname);
	  st_output_thing(&ver,2);
	  st_output_skip(sizeof(SF_PTR) + sizeof(ST_SYMTAB));
	  st_output_skip(sizeof(ST_TABLE)); /* Table for bounds checking */ }
	start_links();
	label_list = (LGSYMBOL **)CALLOC(MAX_LABELS,sizeof(LGSYMBOL *));
}


/***
	finish_up_syms
		This procedure just finishes up the symbol file, doing various fixes
	in the file.
***/

finish_up_syms(filename)
	char *filename;
{
  if (build_freq) return;
	st_output_compat_info(filename);
	do_sym_fix(glbl_tab,HEAD_ST_LOC);
	sym_close();
}

	
/***
	sym_dump
		This procedure outputs a symbol table -- global or local -- to
	the symbol table file.
	
	accepts
		syms -- A pointer to a zap symbol table from which symbol
				information will be taken.
		stoff -- What value to subtract from labels, so that local
					tables will be relative to the beginning of the
					function.  0 for the global table.
					
	returns
		st_syms -- A pointer to a symtab structure, containing information
				   on this table.
***/

int doing_global;
	
ST_SYMTAB
*sym_dump(syms,stoff,is_global)
	STABLE	*syms;
	long	stoff;
int is_global;
{
  DREG1 int i;
  DREG2 int j;
  int process_symbol();
  ST_TABLE n_table;
  ST_TABLE t_table;
  SF_PTR save_loc;
  ST_TABLE s_tables[SPACESUSED];
  ST_SYMTAB *st_syms;
  ST_D_LNK **sort_by_name(),**sort_by_val();
  ST_D_LNK **sorted_bounds;
  long last_val;
  int old_table_num;
  AREG1 STR_FIXUP_NODE *str_list_save;
  AREG2 STR_FIXUP_NODE *temp;
	
  /* Reinitialize lists */
  doing_global = is_global;	/* Used by process_symbol */
  name_list = NULL;
  if (is_global) {
    table_list = NULL;
    table_num = 0; }
  name_num = 0;
  for (i = 0; i < SPACESUSED; i++) {
    space_lists[i] = NULL;
    space_nums[i] = 0;
  }
  if (is_global || !links_init)
    init_links();
	
  /* Initialize the fixup list */
  str_list = NULL;
	
  /* Process symbols */
  stoffset = stoff;
  symwalk(syms,process_symbol);
	
  /* Now sort and output the tables */
  if ((n_table.length = name_num) != 0) {
    n_table.decsptr = symfloc;
    st_output_arr(sort_by_name(name_list,name_num),name_num, 0); }
  else
    n_table.decsptr = 0;
  j = -1;
  for (i = 0; i < SPACESUSED; i++) 
    if ((s_tables[i].length = space_nums[i]) != 0) {
      s_tables[i].decsptr = symfloc;
      st_output_arr(sort_by_val(space_lists[i],space_nums[i]),
		    space_nums[i], 0);
      j = i; }
    else
      s_tables[i].decsptr = 0;
	
  /* Set the symtab header and output the table headers. */
  st_syms = (ST_SYMTAB *)MALLOC(sizeof(ST_SYMTAB));
  st_syms->n_tableptr = symfloc;
  st_output_thing((char *)&n_table,sizeof(ST_TABLE));
  st_syms->num_of_spaces = j + 1;
  st_syms->s_tablesptr = symfloc;
  for (i = 0; i <= j; i++)
    st_output_thing((char *)&(s_tables[i]),sizeof(ST_TABLE));
  
  while (str_list != NULL) {
    str_list_save = str_list;
    str_list_save->header = sym_dump(str_list_save->syms,0,0);
    str_list = str_list_save;
    do_sym_fix(str_list->header,str_list->header_loc);
    temp = str_list;
    str_list = str_list->next;
    FREE(temp->header);
    FREE(temp);
  }
  if (is_global && (table_num > 0)) {
    t_table.decsptr = symfloc;
    sorted_bounds = sort_by_val(table_list, table_num);
    old_table_num = table_num;
    table_num = st_output_arr(sorted_bounds, table_num, 1);
    /* Follow the bounds table with a table full of values, so we
       don't have to read all the decls to get the values */
    last_val = 0xf0000000;
    for (i = 0; i < old_table_num; i++) {
      if (sorted_bounds[i]->sortval.value != last_val) {
	last_val = sorted_bounds[i]->sortval.value;
	st_output_thing((char *)&last_val, sizeof(long)); } }
    t_table.length = table_num;
    save_loc = symfloc;
    st_move_to(BOUNDS_TABLE_LOC);
    st_output_thing((char *)&t_table, sizeof(ST_TABLE));
    st_move_to(save_loc); }
  else
    t_table.decsptr = 0;

  /* Finally, return the symtab pointer */
  return(st_syms);
}


/***
	process_symbol
		This procedure outputs a declaration for each symbol to the syms
	file.  It also adds the symbol to the name list and appropriate
	space list.
	
	accepts
		symbl -- the symbol currently being processed.
***/

process_symbol(symbl)
SYMBOL *symbl;
{
  AREG1 LGSYMBOL *lg_symbl;
  ST_D_LNK *n_link, *s_link, *t_link, *get_link();
  AREG2 char *d_name;
  SF_PTR d_name_loc,d_loc;
  long d_value, val_save;
  int killed = 0;
  
  /* Get various information about the symbol, output info. */
  lg_symbl = (LGSYMBOL *)symbl;
	
  /* If it is a bad symbol, just bail out. */
  if (lg_symbl->lg_space == ANY_SPACE)
    return(SCOK);
  if (lg_symbl->lg_space > SPACESUSED - 1) {
    my_fprintf(stderr,"ZAP: Bad space %d.\n",
	       lg_symbl->lg_space);
    my_exit(SCERR);
  }
  d_name = lg_symbl->lg_sym.sym_name;
  d_name_loc = process_sym_name(d_name);
  d_value = lg_symbl->lg_val.v_value;
  if (is_addr_type(lg_symbl->lg_space))
    d_value = d_value - stoffset;
  if (d_value < 0)  /* Two's complement */
    d_value = ((0 - lg_symbl->lg_val.v_value) ^ ~0L) + 1;
  val_save = d_value;
  /* Make first byte the space #; value limited to 24 bits */
  ((char *)&d_value)[0] = lg_symbl->lg_space; 
  d_loc = symfloc;
  st_output_thing((char *)&d_name_loc,sizeof(SF_PTR));
  st_output_thing((char *)&d_value,sizeof(long));
  process_add_info(lg_symbl);						
  
  /* Set up the links */
  make_new_links(&n_link,&s_link,d_name,d_value,d_loc);
  /* If we're doing the global symbol table, and the symbol we're looking
     at is in one of spaces 1 through 5, add to the list for the bounds-
     checking table.  This maps between actual values and symbols,
     so requires slightly different handling here for each of the different
     spaces. */

  if (doing_global && (val_save >= 0) && (lg_symbl->lg_space >= BOUNDS_TABLE_LOW) &&
      (lg_symbl->lg_space <= BOUNDS_TABLE_HIGH)) {
    if (((lg_symbl->lg_space == TABLE_SPACE) ||
	 (lg_symbl->lg_space == ZSTRING_SPACE)) &&
	(lg_symbl->lg_val.v_flags & ST_KILL)) {
      killed = 1; }
    else {
      t_link = get_link();
      t_link->next = table_list;
      table_list = t_link;
      table_num++;
      t_link->decptr = d_loc;
      if (val_save & 0xff000000)
	printf("Harrumph.\n");
      switch (lg_symbl->lg_space) {
      case TABLE_SPACE:
	if ((!lg_symbl->lg_referenced) &&
	    (d_name[0] != '%') && ((d_name[0] != 'W') || (d_name[1] != '?')))
	  /* Skip half-kills, and things starting with % and W? */
	  my_printf("Unreferenced table: %s\n", d_name);
      case LABEL_SPACE:
	t_link->sortval.value = val_save;
	break;
      case FUNCTION_SPACE:
	if (!lg_symbl->lg_referenced)
	  my_printf("Unreferenced function: %s\n", d_name);
	if (yoffsets)
	  t_link->sortval.value = val_save * 4 + funct_start;
	else
	  t_link->sortval.value = val_save * 4;
	break;
      case ZSTRING_SPACE:
	if (!lg_symbl->lg_referenced)
	  my_printf("Unreferenced string: %s\n", d_name);
	if (yoffsets)
	  t_link->sortval.value = val_save * 4 + string_start;
	else
	  t_link->sortval.value = val_save * 4;
	break;
      case FSTRING_SPACE:
	t_link->sortval.value = val_save * 2; } } }
  else if (doing_global && (lg_symbl->lg_space == VARIABLE_SPACE) &&
	   !lg_symbl->lg_referenced) {
    my_printf("Unreferenced global: %s\n", d_name); }
  n_link->next = name_list;
  name_list = n_link;
  name_num++;
  /* Only put on space list if not half-killed. */
  if ((lg_symbl->lg_val.v_flags & ST_KILL) == 0) {
    s_link->next = space_lists[lg_symbl->lg_space];
    space_lists[lg_symbl->lg_space] = s_link;
    space_nums[lg_symbl->lg_space]++;
  }
  return(SCOK);
}


int
is_addr_type(s)
{
	return((s == LABEL_SPACE));
}


/***
	process_add_info
		This procedure outputs the additional symbol type information, if
	there is any. 
	
	accepts
		sym -- the symbol structure.
***/

process_add_info(sym)
     AREG1 LGSYMBOL *sym;
{
	unsigned char type_info;
  AREG2 TABLE_INFO *temp_tbl;
	STR_FIXUP_NODE *new_struct;
	
		switch (sym->lg_space) {
		case VARIABLE_SPACE:
			type_info = sym->lg_add_info.variable.space;
			st_output_thing(&type_info,1);
			break;
		case FUNCTION_SPACE:
			st_output_thing(&(sym->lg_add_info.function.min_args),1);
			st_output_thing(&(sym->lg_add_info.function.max_args),1);
			st_output_thing(&(sym->lg_add_info.function.value_space),1);
			st_output_skip(1);
			st_output_thing(sym->lg_add_info.function.syms,
							sizeof(ST_SYMTAB));
			break;
		case TABLE_SPACE:
			temp_tbl = sym->lg_add_info.table;
			while (temp_tbl->type == ARRAY_TYPE) {
			  st_output_thing(&(temp_tbl->type),1);
			  st_output_thing(&(temp_tbl->it.array.el_size),
					  sizeof(short));
			  st_output_thing(&(temp_tbl->it.array.num_els),
					  sizeof(short));
			  temp_tbl = temp_tbl->it.array.el_type;
			}
			st_output_thing(&(temp_tbl->type),1);
			if ((temp_tbl->type == WORD_TYPE) ||
			   (temp_tbl->type == BYTE_TYPE))
			   st_output_thing(&(temp_tbl->it.word_or_byte.val_space),1);
			else if (temp_tbl->type == STRUCTURE_TYPE) {
			  new_struct =
			    (STR_FIXUP_NODE *)MALLOC(sizeof(STR_FIXUP_NODE));
			  new_struct->header_loc = symfloc;
			  st_output_skip(sizeof(ST_SYMTAB));
			  new_struct->syms = temp_tbl->it.structure.el_table;
			  new_struct->next = str_list;
			  str_list = new_struct;
			}
			break;
		default:
			break;
		}
}

		
/***
	process_sym_name
		This function searches a hash table for a given symbol name.  If
	it finds it, it returns the syms file location of the name.  If not,
	it adds it to the table, and writes it to the file, returning the
	location.
	
	accepts
		name -- The name of the symbol.
	
	returns
		name_loc -- the pointer to the file position of the name.
***/

process_sym_name(name)
     AREG1 char *name;
{
	unsigned char name_len;
	AREG2 SF_PTR *loc;

	
	loc = (long *)(name - 4);  /* String header contains location. */
	if (*loc == 0) {
	  *loc = symfloc;
	  name_len = strlen(name);
	  st_output_thing((char *)&name_len,1);
	  st_output_thing(name,name_len);
	}
	return (*loc); 
}


/***
	make_new_links
		This procedure creates a name_link and a space_link for a given
	symbol.
	
	accepts
		&n_link -- pointer to name link.
		&s_link -- pointer to space link.
		d_name -- name string.
		d_value -- symbol value.
		d_loc -- symbol's position in syms file.
***/

make_new_links(n_link_p,s_link_p,d_name,d_value,d_loc)
AREG1 ST_D_LNK **n_link_p;
AREG2 ST_D_LNK **s_link_p;
char *d_name;
long d_value;
SF_PTR d_loc;
{
  ST_D_LNK *get_link();
	
  *n_link_p = get_link();
  *s_link_p = get_link();
  (*n_link_p)->decptr = (*s_link_p)->decptr = d_loc;
  (*n_link_p)->sortval.name = d_name;
  (*s_link_p)->sortval.value = d_value;
}

		
/***
	to_array
		This procedure takes a linked list of declarations and converts
	it into an array of declaration pointers, ready to be sorted and
	output (outputted?).
	
	accepts
		d_list -- linked list of declarations.

	returns
		d_array -- pointer to array of declarations.
***/

ST_D_LNK
**to_array(d_list,d_num)
AREG1 ST_D_LNK *d_list;
      unsigned int d_num;
{
	static ST_D_LNK **arr;
	static max_size;
  DREG1 unsigned int i;
  AREG2 ST_D_LNK **d_array;
	if (max_size < d_num) {
	  arr = (ST_D_LNK **)MALLOC(d_num * sizeof(ST_D_LNK *));
	  max_size = d_num;
	}
	d_array = arr;
	for (i = 0; i < d_num; i++) {
	  if (d_list == 0) {
	    printf("Inconsistency in to_array.\n");
	    exit(1); }
	  d_array[i] = d_list;
	  d_list = d_list->next;
	}
	return(d_array);
}
		

/***
	sort_by_name
		This procedure takes an array of declarations and sorts them,
	using a shell sort, by name.
	
	accepts
		d_arr -- array of declarations.
		a_num -- number of elements in the array
		
	returns
		d_arr -- pointer to the sorted array.
***/

ST_D_LNK
**sort_by_name(d_list,a_num)
	ST_D_LNK *d_list;
	unsigned int a_num;
{
  DREG1 int gap;
  DREG2 int i;
  DREG3 int j;
  AREG1 ST_D_LNK **d_arr;
	ST_D_LNK *temp,**to_array();
	
	d_arr = to_array(d_list,a_num);
	for (gap = a_num / 2; gap > 0; gap /= 2)
		for (i = gap; i < a_num; i++)
			for (j = i - gap; j >= 0; j -= gap) {
					if (strcmp(d_arr[j]->sortval.name,
							   d_arr[j + gap]->sortval.name) <= 0)
						break;
					temp = d_arr[j];
					d_arr[j] = d_arr[j + gap];
					d_arr[j + gap] = temp;
			}
	return(d_arr);
}


/***
	sort_by_val
		This procedure takes an array of declarations and sorts them,
	using a shell sort, by value.
	
	accepts
		d_arr -- array of declarations.
		a_num -- number of elements in the array
		
	returns
		d_arr -- pointer to the sorted array.
***/

ST_D_LNK
**sort_by_val(d_list,a_num)
	ST_D_LNK *d_list;
	unsigned int a_num;
{
  DREG1 int gap;
  DREG2 int i;
  DREG3 int j;
  AREG1 ST_D_LNK **d_arr;
	ST_D_LNK *temp,**to_array();
	
	d_arr = to_array(d_list,a_num);
	for (gap = a_num/2; gap > 0; gap /= 2)
		for (i = gap; i < a_num; i++)
			for (j = i - gap; j >= 0; j -= gap) {
					if (d_arr[j]->sortval.value <=
						d_arr[j + gap]->sortval.value)
						break;
					temp = d_arr[j];
					d_arr[j] = d_arr[j + gap];
					d_arr[j + gap] = temp;
			}
	return(d_arr);
}


/***
	output routines
		The following routines actually output information to the syms
	file.  For the moment, inefficient.
	
	st_output_thing
		Outputs a random data object.
		
	st_output_arr
		Outputs syms file pointers from declaration arrays.
	
	st_move_to
		Seeks to given location in file.
	
	st_output_skip
		Writes given number of zeros to file.
***/

static char *symbufs[SBNUM];
static long cursbuf = 0;
static long cursloc = 0;

/***
	sym_open
	
		This procedure opens up the symbol table file.
		
		Accepts
			filename -- the name of the file
***/

int
sym_open(filename)
	char *filename;		/* Name of symbol file. Should be "game.SYMS" */
{
	if ((symfd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0666)) < 0) {
		my_fprintf(stderr, "ZAP: Can not open symbol file \"%s\"\n",filename);
		my_exit(SCNOFILE);
	} else {
		symfloc = 0;
		symbufs[0] = MALLOC(SBSIZE);
	      }
}


/***
	st_output_compat_info()
		This procedure outputs compatibility information to the syms file.
	This consists of a starting location, a length, and data from the 
	object file.
***/

st_output_compat_info(filename)
	char *filename;
{
	int fd;
	char rbuf[COMP_LENGTH];
	SF_PTR save_loc,comp_info_loc;
	short start = COMP_START;
	short length = COMP_LENGTH;
	
	comp_info_loc = symfloc;
	st_output_thing((char *)&start,sizeof(short));
	st_output_thing((char *)&length,sizeof(short));
	if ((fd = open(filename,O_RDONLY)) < 0) {
		my_fprintf(stderr,"Error reading object file for compatibility.\n");
		my_exit(1);
	}
	lseek(fd,COMP_START,0);
	read(fd,rbuf,COMP_LENGTH);
	close(fd);
	st_output_thing(rbuf,COMP_LENGTH);
	save_loc = symfloc;
	st_move_to(COMP_LOC);
	st_output_thing((char *)&comp_info_loc,sizeof(SF_PTR));
	st_move_to(save_loc);
}
	
/***
	do_sym_fix
		This procedure goes back and puts in a symbol table header where it
	belongs.
	
	accepts
		head -- A pointer to the header.
		loc -- Where the header belongs in the syms file.
***/

do_sym_fix(head,loc)
	ST_SYMTAB *head;
	SF_PTR loc;
{
	SF_PTR save_loc;
	
	save_loc = symfloc;
	st_move_to(loc);
	st_output_thing((char *)head,sizeof(ST_SYMTAB));
	st_move_to(save_loc);
}


/***
	sym_close
		This procedure closes and outputsthe symbol file.
***/

sym_close()
{
  DREG1 int i;

  for (i = 0; i < cursbuf; i ++)
    if (write(symfd,symbufs[i],SBSIZE) != SBSIZE) {
      my_fprintf(stderr,"ZAP: Error writing SYMS file.\n");
      my_exit(SCERR);
    }
  if (cursloc != 0)
    if (write(symfd,symbufs[cursbuf],cursloc) != cursloc) {
      my_fprintf(stderr,"ZAP: Error writing SYMS file.\n");
      my_exit(SCERR);
    }
  if (close(symfd) < 0) {
    my_fprintf(stderr,"ZAP: Cannot close syms file.\n");
    my_exit(SCNOFILE);
  }
}


st_output_thing(thing_ptr,thing_len)
  AREG1 char *thing_ptr;
  DREG2 int thing_len;
{	
  DREG1 int i;
  if (build_freq) return;

	for (i = 0; i < thing_len; i++) {
	  symbufs[cursbuf][cursloc++] = thing_ptr[i];
	  if (cursloc == SBSIZE) {
	    if (++cursbuf == SBNUM) {
	      my_fprintf(stderr,"Symbol table file too big; increase constant SBNUM.\n");
	      my_exit(SCERR);
	    }
	    if (symbufs[cursbuf] == NULL)
	      symbufs[cursbuf] = MALLOC(SBSIZE);
	    cursloc = 0;
	  }
	}
        symfloc += thing_len;
}

st_output_arr(dec_arr,arr_len, unique)
AREG1 ST_D_LNK **dec_arr;
unsigned int arr_len;
int unique;
{
  DREG1 unsigned int i;
  SF_PTR temp;
  int ct = 0;
  long last_val = 0xf000000;

  for (i = 0; i < arr_len; i++) {
    if (!unique ||
	(dec_arr[i]->sortval.value != last_val)) {
      last_val = dec_arr[i]->sortval.value;
      ct++;
      temp = dec_arr[i]->decptr;
      st_output_thing((char *)&temp,sizeof(SF_PTR)); }
  }
  return(ct);
}


st_move_to(loc)
	DREG1 SF_PTR loc;
{
	symfloc = loc; /* Note: A move will never go to a new buffer. */
	cursbuf = loc / SBSIZE;
	cursloc = loc % SBSIZE;
}


st_output_skip(num)
	int num;
{
	static char *blank;
	static int size;
	
	if (num > size) {
	  size = num;
	  blank = REALLOC(blank,size);
	}
	st_output_thing(blank,num);
}


/***
	link allocation module
		The following procedures allocate declaration links.  It is set
	up to minimize the trouble of freeing large linked lists.
***/

static ST_D_LNK **dec_pools;
static int cur_pool;
static int next_free_link;

start_links()
{
	dec_pools = (ST_D_LNK **)CALLOC(DEC_POOLS,sizeof(ST_D_LNK *));
}


init_links()
{
  links_init = 1;
  cur_pool = -1; 
  next_free_link = DEC_POOL_SIZE - 1; 
}


ST_D_LNK
*get_link()
{
	if (++next_free_link == DEC_POOL_SIZE) {
	  if (++cur_pool == DEC_POOLS) {
	    my_fprintf(stderr,"Syms: declaration link pool overflow error.\n");
	    return(NULL); }
	  else {
	    next_free_link = 0;
	    if (dec_pools[cur_pool] == NULL) 
	      dec_pools[cur_pool] = 
		(ST_D_LNK *)CALLOC(DEC_POOL_SIZE,sizeof(ST_D_LNK)); } }
	return(&dec_pools[cur_pool][next_free_link]);
}


