/* zsymout.c	Make a symbol table file "game.SYMS"

	Written by Matt Hillman
	
*/

#define __SEG__ Seg2

#include <stdio.h>
#include <fcntl.h>

#include "zap.h"

extern char *MALLOC();
extern char *CALLOC();
extern	STABLE	*symnew();
extern	SYMBOL	*symenter();
extern char **litstr();

extern LGSYMBOL **label_list;
extern unsigned char num_labels;
extern TABLE_INFO *cur_table;
extern unsigned char table_status;
extern	int	Vocreclen;

TABLE_INFO *new_table();
TABLE_INFO *array_table();


/***
	set_table and related functions
	
	This sets all the current labels to an array of the type given.
***/

set_table(info)
	TABLE_INFO *info;
{
	unsigned char i;
	
	for (i = 0; i < num_labels; i++) {
		label_list[i]->lg_space = TABLE_SPACE;
		label_list[i]->lg_add_info.table = info;
	}
	num_labels = 0;
}


TABLE_INFO
*new_table(type,space)
	unsigned char type;
	UBYTE space;
{
	TABLE_INFO *n_table;
	
	n_table = (TABLE_INFO *)MALLOC(sizeof(TABLE_INFO));
	n_table->type = type;
	if ((type == WORD_TYPE) || (type == BYTE_TYPE))
		n_table->it.word_or_byte.val_space = space;
	return(n_table);
}


TABLE_INFO
*array_table()
{
	TABLE_INFO *n_table;
	
	n_table = (TABLE_INFO *)MALLOC(sizeof(TABLE_INFO));
	n_table->type = ARRAY_TYPE;
	n_table->it.array.el_size = 1;
	n_table->it.array.el_type = new_table(BYTE_TYPE,ANY_SPACE);
	return(n_table);
}


set_els(type,space,size)
	unsigned char type,space;
	unsigned short size;
{
	cur_table->it.array.el_type->type = type;
	if ((type == WORD_TYPE) || (type == BYTE_TYPE))
		cur_table->it.array.el_type->it.word_or_byte.val_space = space;
	cur_table->it.array.el_size = size;
	table_status = IN_TABLE;
}


/* This procedure sets up symbol table information to make the last
table encountered the vocabulary table.  It assumes the vocabulary table
is of the following form:
	number of self-inserting break characters (1 byte)
	character #1 (1 byte)
	...
	character #n
	number of bytes per entry (1 byte)
	number of words in vocabulary (1 word)
	.vocbeg statement
	words #1 - #n
	.vocend statement

*/

set_up_vocab(header_size)
	unsigned char header_size;
{
	STABLE *Vocsymtab;
	LGSYMBOL *sym;
	
	if (header_size < 4)
		zerror("Vocabulary table header size too small");
			
	/* First set up the structure table */
	Vocsymtab = symnew(SSHASHSIZE, 0);
	cur_table->type = STRUCTURE_TYPE;
	cur_table->it.structure.el_table = Vocsymtab;
		
	/* First byte in header: # of self-inserting break characters */
	sym = (LGSYMBOL *)symenter(Vocsymtab,*(litstr("num_chars")),sizeof(LGSYMBOL));
	sym->lg_space = TABLE_SPACE;
	sym->lg_val.v_value = 0;
	sym->lg_add_info.table = new_table(BYTE_TYPE,NUMBER_SPACE);
		
	/* Next is array of self-inserting break characters */
	sym = (LGSYMBOL *)symenter(Vocsymtab,*(litstr("chars")),sizeof(LGSYMBOL));
	sym->lg_space = TABLE_SPACE;
	sym->lg_val.v_value = 1;
	cur_table = array_table();
	sym->lg_add_info.table = cur_table;
	set_els(BYTE_TYPE,CHARACTER_SPACE,1);
	cur_table->it.array.num_els = header_size - 4;
		
	/* Number of bytes per entry */
		
	sym = (LGSYMBOL *)symenter(Vocsymtab,*(litstr("rec_size")),sizeof(LGSYMBOL));
	sym->lg_space = TABLE_SPACE;
	sym->lg_val.v_value = header_size - 3;
	sym->lg_add_info.table = new_table(BYTE_TYPE,NUMBER_SPACE);

	/* Number of words in vocabulary */
	sym = (LGSYMBOL *)symenter(Vocsymtab,*(litstr("num_words")),sizeof(LGSYMBOL));
	sym->lg_space = TABLE_SPACE;
	sym->lg_val.v_value = header_size - 2;
	sym->lg_add_info.table = new_table(WORD_TYPE,NUMBER_SPACE);

	/* The words, which will be later counted.*/
	sym = (LGSYMBOL *)symenter(Vocsymtab,*(litstr("words")),sizeof(LGSYMBOL));
	sym->lg_space = TABLE_SPACE;
	sym->lg_val.v_value = header_size;
	cur_table = array_table();
	sym->lg_add_info.table = cur_table;
	set_els(VWORD_TYPE,ANY_SPACE,Vocreclen);
		
	/* Curtable now points to word table */
	table_status = VOCAB_TABLE;
}
