/*	zsymtab.c	Symbol Table Routines

	1988 Zinn Computer Company, for Infocom


Contained in this file:

	symnew		create a new symbol table
	symempty	empty a symbol table (and free the symbols)
	symenter	enter a symbol into a symbol table
	symlookup	lookup a symbol in a symbol table
	symwalk		Apply a function to each symbol in a table

*/

#include <stdio.h>
#include <ctype.h>

#include "zap.h"

/* Local definitions */


/* External routines */

extern char *CALLOC();
extern char *MALLOC();

/* External data */


/* Local routines (and forward references) */


/* Local data */


/* Private data */

/*
 
*//* symnew( hsize, flags )

	Creates a new symbol table

Accepts :

	hsize		number of hash buckets
	flags		Flags for the symbol table

Returns :

	<table>		the new table

*/

STABLE *symnew(hsize, flags)
  int hsize;
  int flags;
{
    STABLE *table;
    SYMBOL **p;
    int n;

    /* allocate the new table */
    table = (STABLE *)CALLOC( 1, sizeof(STABLE) );
    if (table == NULL)
	return (NULL);

    /* allocate the array of hash buckets */
    table->stab_buckets = p = (SYMBOL **)CALLOC( hsize, sizeof(SYMBOL *) );
    if (table->stab_buckets == NULL) {
	CFREE(table, 1, sizeof(STABLE));
	return (NULL);
    }	    

    /* initialize the hash buckets */
    for (table->stab_hsize = n = hsize; --n >= 0; )
	*p++ = NULL;

    /* return the new table */
    table->stab_flags = flags;
    return (table);
}
/*
 
*//* symempty( table, size)

	Frees all of the symbols in a symbol table (but not the table)

Accepts :




	table		a symbol table

Returns :

	<nothing>

*/

symempty(table, size)
  STABLE *table;
  int size;
{
AREG1	SYMBOL *this;
AREG2	SYMBOL *next;

AREG3	SYMBOL **p;
DREG1	int n;

    /* free each hash bucket */
    p = table->stab_buckets;
    for (n = table->stab_hsize; --n >= 0; ) {

	/* free each symbol in this hash bucket */
	for (this = *p; this != NULL; this = next) {
	    next = this->sym_next;
	    if ( ( table->stab_flags & STB_COPYNAME ) != 0 )
		FREE(this->sym_name - 4); /* Include one longword header */
	    CFREE(this,1,size);
	}

	/* clear the bucket */
	*p++ = NULL;
    }
}
/*
 
*//* symenter( table, name, size )

	Enters a symbol in a symbol table (or finds one already there)

Accepts :

	table		the symbol table
	name		the symbol name string
	size		size of the symbol structure

Returns :

	<sym>		the symbol entered (or found)

*/
    
SYMBOL *symenter(table,name,size)
	STABLE *table;		/* the symbol table */
AREG2	char *name;		/* the symbol name */
	int size;		/* size of symbol structure */
{
AREG1	SYMBOL *sym;
AREG3	SYMBOL **p;

    /* get the hash bucket address */
    p = &table->stab_buckets[hash(name,table->stab_hsize)];

    /* look for the symbol */
    for (sym = *p; sym != NULL; sym = sym->sym_next)
	if (strcmp(name,sym->sym_name) == 0)
	    return (sym);

    /* make a new symbol node and link it into the list */
    sym = (SYMBOL *)CALLOC( 1, size );
    if (sym == NULL)
	return (NULL);

    /* if need be, make a copy of the symbol name string */
    if ( ( table->stab_flags & STB_COPYNAME ) != 0 ) {
	sym->sym_name = CALLOC( 1, strlen(name) + 5 );
	if (sym->sym_name == NULL) {
	    CFREE(sym,1,size);
	    return (NULL);
	}
	sym->sym_name += 4; /* One longword header for file location */
	strcpy(sym->sym_name,name);
    }
    else			/* Don't need to copy name */
	sym->sym_name = name;
  
    /* initialize the symbol structure */
    sym->sym_next = *p;
    *p = sym;
    
    /* return the new symbol */
    return (sym);
}
/*
 
*//* symlookup( table, name )

	Find a symbol in a symbol table

Accepts :

	table		the symbol table
	name		the symbol name string

Returns :

	<sym>		the symbol entered (or found)

*/

SYMBOL *symlookup(table,name)
  STABLE *table;		/* the symbol table */
AREG2	char *name;		/* the symbol name */
{
AREG1	SYMBOL *sym;

    /* get the hash bucket */
    sym = table->stab_buckets[hash(name,table->stab_hsize)];

    /* look for the symbol */
    for (; sym != NULL; sym = sym->sym_next)
	if (strcmp(name,sym->sym_name) == 0)
	    return (sym);

    /* can't find it */
    return (NULL);
}
/*
 
*//* symwalk( table, rtc )

	Apply a function to each symbol in a table

Accepts :

	table		the symbol table
	rtc		routine to call

Returns :

	<value>		status code as returned by rtc

Notes :

	rtc is called with the address of the symbol table entry.
	it must return SCOK to continue, or something else to abort.

*/

int symwalk(table, rtc)
  STABLE *table;	/* the symbol table */
  int    (*rtc)();	/* the routine to call */
{
DREG1	int sts;
DREG2   int n;
AREG1	SYMBOL *this;
AREG2	SYMBOL **p;

    /* follow each hash bucket chain */
    p = table->stab_buckets;
    for (n = table->stab_hsize; --n >= 0; ++p ) {

	/* call the routine for each symbol in this chain */
	for (this = *p; this != NULL; this = this->sym_next)
	    if ( ( sts = (*rtc)(this) ) != SCOK )
		return( sts );
    }
    return( SCOK );
}
/*
 
*//* hash( name, size )

	Compute the hash index for a symbol name

Accepts :

	name		the symbol name
	hsize		the hash table size

Returns :

	<index>		the hash index

*/

int hash(name,hsize)
AREG1	char *name;		/* the symbol name */
DREG2	int hsize;		/* the hash table size */
{
DREG1 int i;
AREG2 char valarr[4];

  *(long *)valarr = 0L;
  for (i = 0; *name != '\0'; name++)
    valarr[i++ % 4] ^= *name;
  return (*(long *)valarr % hsize);
}
