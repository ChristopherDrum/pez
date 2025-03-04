/* Program to test symbol table file by Matt Hillman */


#include <stdio.h>
#include <fcntl.h>


#define VARIABLE_SPACE	0
#define LABEL_SPACE		1
#define TABLE_SPACE		2
#define FUNCTION_SPACE	3
#define ZSTRING_SPACE	4
#define FSTRING_SPACE	5
#define OBJECT_SPACE	6
#define PROPERTY_SPACE	7
#define FLAG_SPACE		8
#define VERB_SPACE		9
#define PICTURE_SPACE	10
#define SOUND_SPACE		11
#define CONSTANT_SPACE	12
#define SPACE_SPACE		13
#define ANY_SPACE		254


#define UNKNOWN_TYPE	0
#define BYTE_TYPE		1
#define WORD_TYPE		2
#define ARRAY_TYPE		3
#define STRUCTURE_TYPE	4
#define ZSTRING_TYPE	5
#define OBJECT_TYPE		6
#define PTABLE_TYPE		7
#define VTABLE_TYPE		8

char *malloc();

typedef	struct declaration {
  long	name_loc;
  long	value; } DECLARATION;

char *space_names[] = {"variable", "label", "table", "function", "zstring",
			 "fstring", "object", "property", "flag", "verb",
			 "picture", "sound", "constant", "space"};
#define high_space_name SPACE_SPACE
	
char rbuf[100];
int fd;

main(argc, argv, envp)
int			argc;		/* Argument count */
char		**argv;		/* Argument vector */
char		**envp;		/* Environment vector */
{
  char *filename;
  int length;
  short rshort;
  short symvers;
  char rchar;
  long loc,rlong;
  int command = 99;
	
  if (argc != 2) {
    fprintf(stderr,"Should be only 1 argument, a file name.\n");
    exit(1); }
  filename = argv[1];
	
  if ((fd = open(filename,O_RDONLY)) < 0) {
    fprintf(stderr,"Bad file name.\n");
    exit(1); }
  read(fd, &symvers, sizeof(short));
  printf("Symbol table version %d.\n", symvers);
  read(fd, &rlong, sizeof(char *));
  printf("Compatibility table at %ld.\n", rlong);
  printf("Global symbols:\n");
  read(fd, &rshort, sizeof(short));
  printf("num_spaces %d\n", rshort);
  read(fd, &rlong, sizeof(char *));
  printf(" name_table %ld\n", rlong);
  read(fd, &rlong, sizeof(char *));
  printf(" space_tables %ld\n", rlong);
  if (symvers > 1) {
    read(fd, &rshort, sizeof(short));
    read(fd, &rlong, sizeof(char *));
    printf("Bounds table:  %d at %ld.\n", rshort, rlong); }
  lseek(fd, 0, 0);
  command = 0;
	
  while (command != -1) {
    switch(command) {
    case 0:
      printf("Type a command, followed by a location:\n");
      printf("-1: Quit\n0: Command list\n");
      printf("1: symtab\n2: table\n3: declaration\n4: name\n");
      printf("5: declaration array (follow with length)\n");
      printf("6: new table\n7: new dec\n\n");
      break;
    case 1:
      print_symtab(fd, loc);
      break;
    case 2:
      read(fd,&rshort,sizeof(short));
      printf("table:\n length %d\n",rshort);
      read(fd,&rlong,sizeof(long));
      printf(" declaration %ld\n",rlong);
      break;
    case 3:
      print_declaration(fd, loc);
      break;
    case 4:
      printf("name:\n length %d\n",rchar);
      short_name(fd, loc);
      break;
    case 5:
      scanf("%d",&length);
      printf("declaration array:\n");
      while (length-- > 0) {
	read(fd,&rlong,sizeof(long));
	loc = lseek(fd, 0, 1);
	printf(" %d %ld: ",length,rlong);
	short_decl(fd, rlong);
	lseek(fd, loc, 0); }
      break;
    case 6:
      print_table();
      break;
    case 7:
      print_dec(loc);
      break;
    default:
      break; }
    scanf("%d %ld",&command,&loc);
    lseek(fd,loc,0); } }

short_decl(fd, where)
int fd, where;
{
  long name, val;
  int space;
  char rchar;
  lseek(fd, where, 0);
  read(fd, &name, sizeof(long));
  read(fd, &val, sizeof(long));
  lseek(fd, name, 0);
  read(fd, &rchar, sizeof(char));
  rbuf[rchar] = 0;
  read(fd, rbuf, rchar);
  space = (val >> 24) & 0377;
  printf("%s: %d in %s (%d)\n", rbuf, val & 0xffffff,
	 space>high_space_name?"unknown":space_names[space], space); }

print_declaration(fd, where)
int fd, where;
{
  long name, val;
  int space;
  lseek(fd, where, 0);
  read(fd, &name, sizeof(long));
  read(fd, &val, sizeof(long));
  printf("declaration:\n name %ld: ", name);
  short_name(fd, name);
  space = (val >> 24) & 0377;
  printf(" space %s (%d)\n value %ld\n",
	 space>high_space_name?"unknown":space_names[space], space,
	 val & 0xffffff); }

short_name(fd, where)
int fd, where;
{
  char rchar;
  lseek(fd, where, 0);
  read(fd, &rchar, sizeof(char));
  rbuf[rchar] = 0;
  read(fd, rbuf, rchar);
  printf("%s (len %d)\n", rbuf, rchar); }

print_symtab(fd, where)
int fd, where;
{
  long nametbl, spacetbl;
  short rshort;
  lseek(fd, where, 0);
  read(fd, &rshort, sizeof(short));
  printf("symtab:\n %d spaces\n", rshort);
  read(fd, &nametbl, sizeof(long));
  read(fd, &spacetbl, sizeof(long));
  printf(" name table %ld: ", nametbl);
  short_table(fd, nametbl);
  printf(" space tables %ld\n", spacetbl);
}

short_table(fd, where)
int fd, where;
{
  short len;
  long addr;
  lseek(fd, where, 0);
  read(fd, &len, sizeof(short));
  read(fd, &addr, sizeof(long));
  printf("%d at %ld\n", len, addr); }

/* Given the location of a declaration, this prints the name and value
located there. */

print_dec(loc)
long loc;	
{
  DECLARATION dec;
  char *name;
  unsigned char space;
  long val;
  unsigned char info_byte,type;
  short info_short;

  printf("***\n");
  lseek(fd,loc,0);
  read(fd,&dec,sizeof(DECLARATION));
  read_sym(&name,dec.name_loc);
  space = dec.value >> 24;
  val = dec.value ^ ((long)space << 24);
  printf("\t%ld\t%d\t%ld\t%s\n",loc,space,val,name);
  lseek(fd,loc + sizeof(DECLARATION),0);
  print_space(space);
  switch (space) {
  case VARIABLE_SPACE:
    read(fd,&info_byte,1);
    printf("  %d  ",info_byte);
    print_space(info_byte);
    break;
  case FUNCTION_SPACE:
    read(fd,&info_byte,1);
    printf("min_args %d\n",info_byte);
    read(fd,&info_byte,1);
    printf("max_args %d\n",info_byte);
    read(fd,&info_byte,1);
    printf("value_space %d  ",info_byte);
    print_space(info_byte);
    read(fd,&info_byte,1);
    printf("Function symtab loc %ld\n",
	   loc + sizeof(DECLARATION) + 4);
    break;
  case TABLE_SPACE:
    type = ARRAY_TYPE;
    while (type == ARRAY_TYPE) {
      read(fd,&type,1);
      printf("  Type: %d\n",type);
      switch(type) {
      case BYTE_TYPE:
      case WORD_TYPE:
	read(fd,&info_byte,1);
	printf("  data  %d  ");
	print_space(info_byte);
	break;
      case ARRAY_TYPE:
	read(fd,&info_short,sizeof(short));
	printf("  el_size: %d\n",info_short);
	read(fd,&info_short,sizeof(short));
	printf("  num_els: %d\n",info_short);
	break;
      case STRUCTURE_TYPE:
	printf("Structure table loc: %d\n",
	       loc + sizeof(DECLARATION) + 1);
	break;
      default:
	break; } }
    break;
  default:
    break; }		
  free(name);
  printf("***\n"); }

read_sym(name,loc)
char **name;
long loc;
{
  unsigned char len;
  
  lseek(fd,loc,0);
  read(fd,&len,1);
  *name = malloc(len + 1);
  read(fd,*name,len);
  (*name)[len] = 0; }
	

print_table()
{
  unsigned short len;
  long dec_loc,i,info_loc;
	
  read(fd,&len,sizeof(short));
  read(fd,&dec_loc,sizeof(long));
  for (i = dec_loc; i < dec_loc + (len * sizeof(long)); i += sizeof(long)) {
    lseek(fd,i,0);
    read(fd,&info_loc,sizeof(long));
    printf("%ld ",i);
    print_dec(info_loc); } }


print_space(space)
unsigned char space;
{
  printf("Space: ");
  switch (space) {
  case VARIABLE_SPACE:
    printf("variable\n");
    break;
  case LABEL_SPACE:
    printf("label\n");
    break;
  case TABLE_SPACE:
    printf("table\n");
    break;
  case FUNCTION_SPACE:
    printf("function\n");
    break;
  case ZSTRING_SPACE:
    printf("zstring\n");
    break;
  case FSTRING_SPACE:
    printf("fstring\n");
    break;
  case OBJECT_SPACE:
    printf("object\n");
    break;
  case PROPERTY_SPACE:
    printf("property\n");
    break;
  case FLAG_SPACE:
    printf("flag\n");
    break;
  case VERB_SPACE:
    printf("verb\n");
    break;
  case PICTURE_SPACE:
    printf("picture\n");
    break;
  case SOUND_SPACE:
    printf("sound\n");
    break;
  case CONSTANT_SPACE:
    printf("constant\n");
    break;
  case SPACE_SPACE:
    printf("space\n");
    break;
  case ANY_SPACE:
    printf("any\n");
    break;
  default:
    printf("unknown %d\n",space);
    break; } }
