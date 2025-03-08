/* Split program
   By Matt Hillman

   This program takes a .zip file and a .seg file. It makes .dsx files where x
is the disk number.
   Using the segment information the program divides the game among several disks in 
the most efficient way possible.

*/

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include "splitdefs.h"

#define MAPEST 3       /* Estimate # of pages needed for disk map */

#define FALSE 0
#define TRUE  1

/* Segment flags */

#define SF_ADJ 0x01   /* Whether the segment is on any disk with an adjacent seg */
#define SF_USED  0x02 /* Whether the segment has been written to disk or not */

/* Disk flags */

#define DF_NEEDED    0x01  /* Whether the disk is "needed." This isn't used. */

typedef
  struct segment {
    char *name;                   /* The segment's name */
    unsigned char flags;          /* Segment flags */
    unsigned short lastdisk;      /* Last disk on which segment was attempted. */
    unsigned short lastsucd;      /* Last disk on which segment write was successful */
    unsigned short lastcurd;      /* Last disk for which segment was current (curseg) */
    unsigned short lastfild;      /* Same as above, but disk filling (fillseg) */
    int seg_size;		/* Size of segment in pages */
    long seg_pic_size;		/* Bytes for segment's pictures */
    char pages[PAGENUM / 8];      /* Page bit table */
  } SEGMENT;

typedef
  struct disk {
    unsigned char flags;          /* Disk flags.  Not currently used */
    long pix;		/* Number of pages used by pictures */
    unsigned short free;          /* Number of pages on disk available for segments */
    char pages[PAGENUM / 8];      /* Page bit table */
    unsigned short last_cur;	/* Last segment current on this disk */
  } DISK;

char *pic_segs = 0;
int global_pix = 0;
int global_pix_disk = -1;
int seg0_pix = 0;
char *hint_segment_name = 0;
char *dup_segment_name = 0;
SEGMENT *hint_segment = 0;		/* May be treated specially:  put on first
				   disk along with preload */
SEGMENT *dup_segment = 0;
char *dup_segment_pages = 0;

char *concat();
char *makemap();
unsigned short funct_page,endload,top_page;  /* Values from .seg file */
int disksize;
unsigned short precheck = 0;   /* Checksum of preload */
char disk0 = FALSE;            /* Add segments on disk 0? */
char vpreload = TRUE;          /* Variable-size preload? */
int preload = 0;               /* Size of preload */
int extra_preload = 0;		/* Size of extra preload for apple II */
int npre_page = 0;
int active_disks = 1;		/* Number of disk drives assumed
				   If not 1, segment 0 only lives in one place,
				   and no page from it is duplicated on any
				   other disk. */
int allowed_disks = 0;		/* If non-zero, the number of disks/surfaces
				   we'll generate.  4 is a nice number for,
				   say, apple II zork 0.
				   If active_disks is 1, this is more or less
				   irrelevant, since we have no leeway. */
int d0size = 0;                /* Space available on disk 0 */
char filename[256];
int fnamelen;
int zipfd,segfd,dskfd;         /* file descriptors */
short numsegs;                 /* Number of segments */
int segs_remaining;		/* Segments not on any disk */
char concatpages[PAGENUM / 8]; /* Default table used by concat procedure */
char preload_tab[PAGENUM / 8]; /* Preload page table */
int funny_disk_0 = 0;
char all_pages[PAGENUM / 8];	/* To tell us whether pages didn't get written */
SEGMENT         *segments;     /* Array of segments */
DISK            *disks;        /* Array of disks */
char            *adjacents;    /* Adjacency table */
unsigned char   *objfile;      /* Object code array */
long            objlen;        /* Length of object code */

/* Bit array for using bit tables.  See zapdoc.txt */
static char bit[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};


/* The main procedure reads in the command line */

char *do_calloc();

int topages(num)
long num;
{
  return((num + PAGESIZE - 1) / PAGESIZE); }

int left_on_disk(disk)
DISK* disk;
{
  return(disk->free - (amt(disk->pages) + topages(disk->pix))); }

int used_after_segment(disk, segment, np)
DISK *disk;
SEGMENT *segment;
char **np;
{
  char *newpages;
  int new_data;
  if (np == NULL)
    newpages = concat(disk->pages, segment->pages, NULL);
  else {
    newpages = concat(disk->pages, segment->pages, *np);
    *np = newpages; }
  new_data = amt(newpages);
  return(new_data
	 + topages(disk->pix + segment->seg_pic_size)); }

/* Return the disk map itself */
char
*makemap(maplen,mappages,numdisks)
     unsigned short maplen;
     unsigned short numdisks, mappages;
{
  unsigned short *map;
  unsigned short i,j,offset,checksum,numblocks, tot_blocks = 0;
  short start;
  int checkloc;
  int mapword = MAP_HEADER_SIZE;
  
  printf("Making disk map...\n");
  map = (unsigned short *)do_calloc(1,mappages * PAGESIZE);
  for (i = 0; i < MAP_HEADER_SIZE; i++)
    map[i] = 0;
  map[MH_MAPLEN] = maplen - 1;  /* Number of words following */
  map[MH_DISK_COUNT] = numdisks;
  
  for (i = 0; i < numdisks; i++) {
    if (i == 0) {
      offset = mappages;
      checksum = 0; }
    else {
      offset = 0;
      checksum = 0; }
    start = -1;
    numblocks = 0;
    checkloc = mapword; /* Save space for checksum, number of blocks */
    map[mapword+DH_CHECKSUM] = 0;		/* Checksum */
    map[mapword+DH_PICTURE_PAGE] = 0;		/* Address of pictures */
    map[mapword+DH_BLOCKS] = 0;		/* number of blocks */
    if (i == global_pix_disk)
      map[mapword+DH_GLOBAL_PIX] = -1;
    else
      map[mapword+DH_GLOBAL_PIX] = 0;
    mapword += DISK_HEADER_SIZE;
    if (!i && preload && funny_disk_0) {
      for (j = 0; j <= top_page; j++) {
	if (((preload_tab[j / 8] & bit[j % 8]) != 0) && (start == -1))
	  start = j;
	if ((start != -1) &&
	    ((j == top_page) || ((preload_tab[j / 8] & bit[j % 8]) == 0))) {
	  if ((preload_tab[j / 8] & bit[j % 8]) != 0)
	    j++;
	  printf("Disk 1: Pages %d to %d starting on disk page %d.\n",
		 start, j - 1, offset);
	  map[mapword+B_START] = start;
	  map[mapword+B_END] = j - 1;
	  map[mapword+B_PAGE] = offset;
	  mapword += BLOCK_SIZE;
	  offset += j - start;
	  checksum += check(start, j - 1);
	  start = -1;
	  numblocks++;
	  tot_blocks++; } }
      start = -1;
      for (j = 0; j <= top_page; j++) {
	if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	    ((preload_tab[j / 8] & bit[j % 8]) == 0) && (start == -1) &&
	    ((i != 1) || !extra_preload || (j < npre_page) ||
	     (j >= (npre_page + extra_preload))))
	  start = j;
	if ((start != -1) &&
	    ((j == top_page) || ((disks[0].pages[j / 8] & bit[j % 8]) == 0) ||
	     ((preload_tab[j / 8] & bit[j % 8]) != 0) ||
	     ((i == 1) && extra_preload && (j >= npre_page) &&
	      (j < (npre_page + extra_preload))))) {
	  if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	      ((preload_tab[j / 8] & bit[j % 8]) == 0))
	    j++;
	  printf("Disk 1: Pages %d to %d starting on disk page %d.\n",
		 start, j - 1, offset);
	  map[mapword+B_START] = start;
	  map[mapword+B_END] = j - 1;
	  map[mapword+B_PAGE] = offset;
	  mapword += BLOCK_SIZE;
	  offset += j - start;
	  checksum += check(start, j - 1);
	  start = -1;
	  numblocks++;
	  tot_blocks++; } } }
    else {
      if ((i == 1) && extra_preload) {
	printf("Disk 2: Pages %d to %d starting on disk page 0.\n",
	       npre_page, npre_page + extra_preload - 1);
	map[mapword+B_START] = npre_page;
	map[mapword+B_END] = npre_page + extra_preload - 1;
	map[mapword+B_PAGE] = offset;
	mapword += BLOCK_SIZE;
	offset += extra_preload;
	checksum += check(npre_page, npre_page + extra_preload - 1);
	numblocks++;
	tot_blocks++; }
      for (j = 0; j <= top_page; j++) {
	if (((disks[i].pages[j / 8] & bit[j % 8]) != 0) &&
	    (start == -1) &&
	    ((i != 1) || !extra_preload ||
	     (j < npre_page) || (j >= (npre_page + extra_preload))))
	  start = j;  /* Start a block; existing page after non-existing page */
	if ((start != -1) &&
	    ((j == top_page) || ((disks[i].pages[j / 8] & bit[j % 8]) == 0) ||
	     ((i == 1) && extra_preload &&
	      (j == npre_page)))) {
	  if (((disks[i].pages[j / 8] & bit[j % 8]) != 0) &&
	      ((i != 1) || !extra_preload || (j < npre_page) ||
	       (j >= (npre_page + extra_preload))))
	    j++;   /* j was top page */
	  printf("Disk %d: Pages %d to %d starting on disk page %d.\n",
		 i + 1,start,j - 1,offset);
	  map[mapword+B_START] = start;
	  /* Block ended; output start,end, and disk page */
	  map[mapword+B_END] = j - 1;
	  map[mapword+B_PAGE] = offset;
	  mapword += BLOCK_SIZE;
	  offset += j - start;
	  checksum += check(start,j - 1);
	  start = -1;               /* No longer in a block */
	  numblocks++;
	  tot_blocks++;
      } } }
    map[checkloc+DH_CHECKSUM] = checksum;  /* Fill in checksum and... */
    map[checkloc+DH_BLOCKS] = numblocks;   /* number of blocks */
  }
  printf("%d blocks (%d bytes).\n", tot_blocks, BLOCK_SIZE * tot_blocks * 2);
  return((char *)map);   /* Return the map */
}

char *do_malloc(sz)
int sz;
{
  char *buf;
  buf = (char *)malloc(sz);
  if (!buf) {
    printf("Malloc of %d failed.\n", sz);
    exit(1); }
  return(buf); }

char *do_calloc(n, sz)
int n, sz;
{
  char *buf;
  register int i;
  sz = n * sz;
  buf = do_malloc(sz);
  for (i = 0; i < sz; i++)
    buf[i] = 0;
  return(buf); }

void usage()
{
  fprintf(stderr, "Usage: zsplit [-p n] [-s n] [-d n] [-a n] fname disksize\n");
  fprintf(stderr,
	  "-p is preload size; -s is space available on disk 1. Exactly one must be\n");
  fprintf(stderr, "   supplied.\n");
  fprintf(stderr,
	  "-a is number of disk drives assumed; 2 is the only interesting argument.\n");
  fprintf(stderr,
	  "-d is number of disks we'll build.  This is only meaningful if -a is 2,\n");
  fprintf(stderr, "    because we don't have any leeway otherwise.\n"); }

main(argc,argv,envp)
int		argc;		/* Argument count */
char		**argv;		/* Argument vector */
char		**envp;		/* Environment vector */
{
  int tfile;
  int i;		/* Scratch */
  int n;		/* Scratch again */
  int c;		/* A character */
  char *aptr, *tptr;
  int na;		/* number of args on command line */

  na = 0;			/* No files on command line */

  print_comptime("zsplit");
      
  /* Collect command line arguments before doing anything */
  for(i = 1; i < argc; ++i) {
    /* Check for command line option.  We'll allow the unix
       style "-x" form as well as the original ZAP "/x" form. */
    if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
      c = argv[i][1];
      if (isupper(c))		/* Allow either-case */
	c = tolower(c);
      n = i;			/* Assume value is bound to option */
      aptr = NULL;		/* Now verify that */
      if (argv[i][2] != NULL) {
	if (argv[i][2] == '=') {
	  if (argv[i][3] != NULL)
	    aptr = &argv[i][3];	}
	else			/* No "=" after option char */
	  aptr = &argv[i][2]; }
      if (aptr == NULL) {	/* If no argument with option */
	if (i < argc - 1) {	/*  use next token as arg, if any */
	  ++n;
	  aptr = argv[n]; } }
      /* We're finally ready to process the option char */
      switch (c) {
      case 'e':
	extra_preload = atoi(aptr);
	printf("Extra preload %d\n", extra_preload);
	i = n;
	break;
      case 'p':        /* Toggle variable preload (have set preload) */
	vpreload = !vpreload;
	preload = atoi(aptr);
	printf("Preload size %d\n", preload);
	i = n;
	break;
      case 's':  /* Toggle use of disk 1 (use it) */
	disk0 = !disk0;
	d0size = atoi(aptr);
	printf("Disk 1 %s\n", disk0?"on":"off");
	i = n;
	break;
      case 'd':
	allowed_disks = atoi(aptr); /* Number of disks to build */
	printf("%d disks allowed\n", allowed_disks);
	i = n;
	break;
      case 'a':
	active_disks = atoi(aptr); /* Active disks */
	printf("%x disks active\n", active_disks);
	i = n;
	break;
      case '0':
	seg0_pix = 1;
	printf("Segment 0 pictures go with segment 0.\n");
	break;
      case 'u':
	tfile = open(aptr, O_RDONLY);
	if (tfile >= 0) {
	  global_pix = lseek(tfile, 0, 2);
	  close(tfile);
	  global_pix = (global_pix + PAGESIZE - 1) / PAGESIZE; }
	else {
	  perror(aptr);
	  global_pix = 1;
	  printf("Assuming 1 page global directory.\n");
	  i = n;
	  break; }
	printf("Global picture directory from %s:  %d pages.\n", aptr, global_pix);
	i = n;
	break;
      case 'g':
	pic_segs = aptr;
	printf("Picture sizes from %s\n", pic_segs);
	i = n;
	break;
      case 'c':
	tptr = aptr;
	while (*tptr) {
	  if ((*tptr >= 'a') && (*tptr <= 'z'))
	    *tptr = *tptr + 'A' - 'a';
	  tptr++; }
	dup_segment_name = aptr;
	i = n;
	printf("Segment %s will be on all sides other than 1 and 2\n", aptr);
	break;
      case 'h':
	tptr = aptr;
	while (*tptr) {
	  if ((*tptr >= 'a') && (*tptr <= 'z'))
	    *tptr = *tptr + 'A' - 'a';
	  tptr++; }
	hint_segment_name = aptr;
	printf("Hint segment is called %s\n", hint_segment_name);
	i = n;
	break;
      default:			/* Dunno what it is! */
	fprintf(stderr, "Ignoring unknown option %s\n",argv[i]);
	break; } }
    else {			/* Not an option */
      switch(na++) {
      case 0:			/* Filename */
	strcpy(filename,argv[i]);
	break;
      case 1:			/* Size of disk */
	disksize = atoi(argv[i]);
	break;
      default:
	usage();
	exit(1);
	break; } } }
  /* Command line is in. */
  if (na != 2) {
    usage();
    exit(1); }
  if ((vpreload) && (!disk0)) {
    fprintf(stderr,"Either -p or -s must be chosen.\n");
    usage();
    exit(1); }
  fnamelen = strlen(filename);
  if ((strcmp(&filename[fnamelen - 4], ".zip") != 0) &&
      (strcmp(&filename[fnamelen - 4], ".ZIP") != 0))
    strcat(filename, ".zip");	/* Add the zip */
  fnamelen = strlen(filename);
  if ((zipfd = open(filename,O_RDONLY)) < 0) {
    fprintf(stderr,"Can't open .zip file.\n");
    exit(1); }
  filename[fnamelen - 4] = NULL;
  strcat(filename,".seg");
  if ((segfd = open(filename,O_RDONLY)) < 0) {
    fprintf(stderr,"Can't open .seg file.\n");
    exit(1); }
  presplit(MAPEST);
}

void load_pic_segs(name)
char *name;
{
  int fd;
  long sz;
  int i, won;
  char *buf, *str, *nstr, *nbuf;
  fd = open(name, O_RDONLY);
  if (fd < 0) {
    perror("Can't open picture segment file");
    return; }
  sz = lseek(fd, 0, 2);
  lseek(fd, 0, 0);
  buf = (char *)do_malloc(sz);
  read(fd, buf, sz);
  close(fd);
  nbuf = buf;
  while (1) {
    str = strtok(nbuf, " \n");
    if (!str) break;
    nbuf = (char *)0L;
    nstr = strtok(nbuf, " \n");
    sz = atoi(nstr);
    i = 0;
    while (str[i]) {
      if ((str[i] >= 'a') && (str[i] <= 'z'))
	str[i] = str[i] - 'a' + 'A'; /* Uppercase the segment name */
      i++; }
    won = 0;
    for (i = 0; i < numsegs; i++) {
      if (strcmp(str, segments[i].name) == 0) {
	segments[i].seg_pic_size = sz;
	won = 1;
	break; } }
    if (!won) {
      printf("Can't find segment %s.\n", str); } }
  free(buf); }

/* This procedure reads in the input files and deals with the preload. 
See zsegment.c for file formats */
presplit(mapest)
int mapest;
{
  int i,j;         
  char readbuf[256];  /* Used for reading segment names */
  char c1, c2;
  int numdisks;       /* Number of disks needed */

  printf("Splitting %s...\n",filename);
  if (preload + mapest > disksize) {
    printf("Preload size too big for disk size.\n");
    exit(1); }
  read(segfd,&numsegs,2);   /* Read stuff from .seg file */
  segs_remaining = numsegs;	/* Number left to place */
  read(segfd,&endload,2);
  read(segfd,&funct_page,2);
  read(segfd,&top_page,2);
  printf("Segments: %d\nPages: %d\n",numsegs,top_page + 1);
  if (funct_page == endload) /* Don't repeat a page */
    ++funct_page;  /* Increment function start page for purposes of preload */
  adjacents = do_calloc(1,(numsegs * numsegs + 7) / 8);
  segments = (SEGMENT *)do_calloc(numsegs,sizeof(SEGMENT));
  disks = (DISK *)do_calloc(numsegs,sizeof(DISK));
  read(segfd,adjacents,(numsegs * numsegs + 7) / 8); /* Read adjacency table */
  for (i = 0; i < numsegs; i++) {
    segments[i].flags = 0;
    readbuf[255] = 0;
    for (j = 0; j < 255; j++) {
      read(segfd,readbuf + j,1);   /* Read segment name */
      if (readbuf[j] == '\0')
	break; }
    segments[i].name = do_malloc(strlen(readbuf) + 1);
    strcpy(segments[i].name,readbuf);
    if (hint_segment_name) {
      if (strcmp(hint_segment_name, segments[i].name) == 0)
	hint_segment = &segments[i]; }
    if (dup_segment_name) {
      if (strcmp(dup_segment_name, segments[i].name) == 0) {
	dup_segment = &segments[i];
	dup_segment_pages = &segments[i].pages[0]; } }
    read(segfd,segments[i].pages,PAGENUM / 8); /* Read segment page table */ }
  if (hint_segment_name && !hint_segment)
    printf("Hint segment %s not found.\n", hint_segment_name);
  if (dup_segment_name && !dup_segment)
    printf("Copied segment %s not found.\n", dup_segment_name);
  if (pic_segs)
    load_pic_segs(pic_segs);
  for (i = 0; i < numsegs; i++)
    display_segment(i);
  if (close(segfd) < 0) {
    fprintf(stderr,"Cannot close SEG file.\n");
    exit(); }
  objfile = (unsigned char *)do_calloc(1,(top_page + 1) * PAGESIZE);
  /* Read object file into big array */
  if ((objlen = read(zipfd,objfile,(top_page + 1) * PAGESIZE)) < 0) {
    fprintf(stderr,"Error reading object file.\n");
    exit(1); }
  printf("Object bytes read in: %ld\n",objlen);
  close(zipfd);
  if (preload > top_page + 1) {  /* Don't need to do split */
    printf("Preload bigger than object code; all code fits in preload.\n");
    preload = top_page + 1; }
  if (preload == 0)
    printf("No preload.\n");
  else {
    if (preload - 1 <= endload) { /* Preload just taken from start of file */
      npre_page = funct_page;	/* First page for extra preload, if used */
      printf("Putting pages 0 - %d in preload.\n",preload - 1);
      funny_disk_0 = 1;
      addpages(0,preload - 1,preload_tab);
      addpages(0, preload - 1, disks[0].pages); }
    else {  /* Otherwise, preload taken from o-endload, then from function space */
      npre_page = funct_page + preload - endload - 1;
      printf("Putting pages 0 - %d and %d - %d in preload.\n",
	     endload,funct_page, npre_page - 1);
      funny_disk_0 = 1;
      addpages(0,endload,preload_tab);
      addpages(0, endload, disks[0].pages);
      addpages(funct_page, npre_page - 1, preload_tab);
      addpages(funct_page, npre_page - 1, disks[0].pages); } }
  for (i = 0; i < numsegs; i++) {
    if (!vpreload) /* Remove preload pages from segments */
      delpages(segments[i].pages,preload_tab);
    disks[i].free = disksize;
    segments[i].lastdisk = numsegs; }
  if (extra_preload) {
    printf("Putting pages %d - %d in extra preload.\n", npre_page,
	   npre_page + extra_preload - 1);
    addpages(npre_page, npre_page + extra_preload - 1, disks[1].pages);
    for (i = 0; i < numsegs; i++) {
      /* Delete the pages from any segment they appear in, except hints
	 and duplicates; they're already on disk 1, so don't need to be
	 anywhere else. */
      if ((&segments[i] != hint_segment) &&
	  (&segments[i] != dup_segment)) {
	delpages(segments[i].pages, disks[1].pages); } } }
  if (disk0) { /* Room is available on disk 0 */
    disks[0].free = d0size - mapest; }
  if (vpreload) /* Implies disk0 option */
    if (endload < disks[0].free) {
      printf("Putting impure and parser tables (pages 0 to %d) on disk 1.\n",endload);
      addpages(0,endload,disks[0].pages);         /* Add pages to disk 0... */
      delpages(segments[0].pages,disks[0].pages); } /* Delete them from segment 0 */
    else {
      printf("Oops! Impure and parser tables don't fit on disk 1.\n");
      exit(1); }
  if (preload < top_page + 1)
    numdisks = split();    /* Split segments into disks using some algorithm. */
  else
    numdisks = 1;  /* Don't need to do split */
  makedisks(numdisks,mapest);   /* Time to make the disks */
}


display_segment(i)
int i;
{
  int j, start = -1;
  int size = 0;
  int pno, bno;
  printf("Segment %d:  %s\nPages ", i, segments[i].name);
  for (j = 0; j < PAGENUM; j++) {
    pno = j / 8;
    bno = j % 8;
    if ((segments[i].pages[pno] & bit[bno]) &&
	((i == 0) || (&segments[i] == hint_segment) || (&segments[i] == dup_segment) ||
	 !(segments[0].pages[pno] & bit[bno]))) {
      size++;
      if (start < 0)
	start = j; }
    else if (start >= 0) {
      if (start == (j - 1))
	printf("%d ", start);
      else
	printf("%d-%d ", start, j - 1);
      start = -1; } }
  segments[i].seg_size = size;
  printf("\n%d pages, %d picture bytes\n", size, segments[i].seg_pic_size); }

flush_from(amount, s0_disk, first, last, temppages, stop)
int amount, first, last, stop, s0_disk;
char *temppages;
{
  char used_pages[PAGENUM / 8];
  int i;
  int old_left, new_left, mask = 0;
  if (stop) {
    /* We can't use all the matches found, so just go until done. */
    int pno, bno;
    amount = 0;
    for (i = 0; i < (PAGENUM / 8); i++)
      used_pages[i] = 0;
    for (i = 0; i < PAGENUM; i++) {
      pno = i / 8;
      bno = i % 8;
      if (temppages[pno] & bit[bno]) {
	addpages(pno, pno, used_pages);
	stop--;
	amount++;
	if (stop <= 0) break; } }
    temppages = &used_pages[0]; }
  printf("Moving %d pages to %d.\n", amount, s0_disk + 1);
  for (i = first; i < last; i++) {
    old_left = left_on_disk(&disks[i]);
    delpages(disks[i].pages, temppages);
    new_left = left_on_disk(&disks[i]);
    if (old_left != new_left) mask |= 1 << i;
    printf("%d left on %d.\n", left_on_disk(&disks[i]), i + 1); }
  for (i = 0; i < numsegs; i++) {
    if ((&segments[i] != hint_segment) && (&segments[i] != dup_segment)) {
      delpages(segments[i].pages, temppages);
      segments[i].seg_size = amt(segments[i].pages); } }
  return(mask); }

do_common(s0_disk)
int s0_disk;
{
  static char temppages[PAGENUM / 8];
  int got_some = 0;
  int i, j, k, pno, bno, amount;
  int cur_left;
  int good_disk = -1, best_left = 0;
  if (left_on_disk(&disks[s0_disk]) > 0) {
    for (i = s0_disk + 1; i < allowed_disks; i++) {
      cur_left = left_on_disk(&disks[i]);
      if (cur_left > best_left) {
	best_left = cur_left;
	good_disk = i; } }
    if (good_disk < 0) good_disk = s0_disk + 1;
    for (i = s0_disk + 1; i < (allowed_disks - 1); i++) {
      if ((cur_left = left_on_disk(&disks[s0_disk])) <= 0) break;
      for (j = i + 1; j < allowed_disks; j++) {
	if ((i != good_disk) && (j != good_disk)) continue;
	amount = andpages(temppages, disks[i].pages, disks[j].pages);
	if (amount > 0) {
	  if (dup_segment) {
	    delpages(temppages, dup_segment_pages);
	    /* Don't move pages from this segment to segment 0 disk */
	    amount = amt(temppages);
	    if (amount <= 0) continue; }
	  if (amount <= cur_left) {
	    got_some |= flush_from(amount, s0_disk, s0_disk + 1,
				   allowed_disks, temppages, 0);
	    concat(disks[s0_disk].pages, temppages, disks[s0_disk].pages);
	    printf("%d left on %d.\n", left_on_disk(&disks[s0_disk]),
		   s0_disk + 1); }
	  else {
	    amount = 0;
	    got_some |= flush_from(amount, s0_disk, s0_disk + 1, allowed_disks,
				   temppages, cur_left);
	    printf("%d left on %d.\n", left_on_disk(&disks[s0_disk]),
		   s0_disk + 1); } } } } }
  return(got_some); }

int last_disk_used = 0;

/* This procedure actually splits the segments into disks. See zsplitdoc.txt for
split algorithm.
   In general : FILLING a disk refers to adding already used segments to the disk. */
split()
{
  char temppages[PAGENUM / 8];
  int amount;
  int last_disk = 0;
  int got_some = 0;
  char done = FALSE;   /* TRUE if done splitting */
  char donefill;       /* TRUE if we are done filling a disk */
  int disknum = 0;     /* Disk we are currently making */
  int curseg = 0;		/* Disk we are currently looking for adjacents to */
  int tmp;
  int fillseg;         /* Disk we are looking for adjacents to while filling */
  int sts = 0;
  int i, j, k, pno, bno, bestdisk, bestleft;
  int pages_left;
  int biggest_disk, biggest_size, second_disk, second_size, delta;
  int target_pages;
  int seg0_disk = -1;
  int base_any_on_disk = 1;
  char any_on_disk = 1;
  int seg0_disk_used = 0;
  int cur_left;
  int times_through, old_remaining = segs_remaining;
  if (!disk0) {
    disknum++;  /* Skip disk 0 */
    last_disk = disknum; }
  else if (vpreload) {
    if (addseg0(0) >= numsegs) {
      printf("Segment 0 doesn't fit on disk 1; must be replicated on all surfaces.\n");
      active_disks = 1; }
    else
      seg0_disk = disknum; }
  if (active_disks > 1) {
    if (seg0_disk < 0) {
      seg0_disk = disknum;
      segs_remaining--;
      if (dup_segment)
	delpages(segments[0].pages, dup_segment_pages);
      addseg0(disknum); }
    disknum++;
    last_disk = disknum;
    for (i = 1; i < numsegs; i++) {
      if ((&segments[i] != hint_segment) && (&segments[i] != dup_segment))
	/* Don't delete pages shared with segment 0 if it's the hint segment */
	delpages(segments[i].pages, segments[0].pages); } }
  if ((active_disks > 1) && allowed_disks) {
    /* Alternate algorithm if we're assuming two drives.
       Segment 0 goes on the 'first' disk (if vpreload, disk 0;
       otherwise, disk 1).  If allowed_disks is defined, other stuff is split
       fairly evenly; otherwise, we just fill up as many disks as possible
       (which will tend not to leave any space for pix). */
    pages_left = top_page - preload - (disks[seg0_disk].free -
				       left_on_disk(&disks[seg0_disk]));
    if (hint_segment) {
      segs_remaining--;
      hint_segment->lastdisk = 0;
      concat(disks[0].pages, hint_segment->pages, disks[0].pages);
      hint_segment->flags |= SF_USED;
      disks[0].pix += hint_segment->seg_pic_size;
      printf("Adding hint segment %s to disk 1; total size now %d.\n",
	     hint_segment->name, amt(disks[0].pages)); }
    if (dup_segment) {
      segs_remaining--;
      dup_segment->lastdisk = allowed_disks - 1;
      dup_segment->flags |= SF_USED;
      for (i = seg0_disk + 1; i < allowed_disks; i++) {
	concat(disks[i].pages, dup_segment_pages, disks[i].pages);
	printf("Adding copied segment %s to disk %d; total size now %d.\n",
	       dup_segment->name, i+1, amt(disks[i].pages)); } }
    /* Get number of pages we have to divide up.  Note that duplication of
       pages between disks may increase the actual number of pages used. */
    if (seg0_pix) {
      disks[seg0_disk].pix = segments[0].seg_pic_size;
      printf("Adding segment 0 pictures to %d; total size now %d.\n",
	     seg0_disk + 1, disks[seg0_disk].free - left_on_disk(&disks[seg0_disk]));
      if (global_pix) {
	disks[seg0_disk].pix += (global_pix * PAGESIZE);
	printf("Adding global picture directory to %d; total size now %d.\n",
	       seg0_disk + 1, disks[seg0_disk].free - left_on_disk(&disks[seg0_disk]));
	global_pix_disk = seg0_disk; } }
    else {
      for (i = seg0_disk + 1; i < allowed_disks; i++) {
	disks[i].pix = segments[0].seg_pic_size;
	printf("Adding segment 0 pictures to %d; total size now %d.\n",
	       i + 1, disks[i].free - left_on_disk(&disks[i])); }
      if (global_pix) {
	i = seg0_disk + 1;
	disks[i].pix += (global_pix * PAGESIZE);
	global_pix_disk = i;
	printf("Adding global picture directory to %d; total size now %d.\n",
	       i + 1, disks[i].free - left_on_disk(&disks[i])); } }
    target_pages = 0;
    /* target_pages = disksize - (pages_left / (allowed_disks - disknum)); */
    /* Maximum number of pages left on each disk */
    while (!done) {
      if (segs_remaining == old_remaining) {
	if (++times_through > allowed_disks) {
	  if (left_on_disk(&disks[seg0_disk]) > 0) {
	    cur_left = left_on_disk(&disks[seg0_disk]);
	    /* We can get more room by moving stuff to segment 0 disk */
	    bestleft = 0;
	    bestdisk = -1;
	    for (i = seg0_disk + 1; i < allowed_disks; i++) {
	      if (left_on_disk(&disks[i]) >= bestleft) {
		bestleft = left_on_disk(&disks[i]);
		bestdisk = i; } }
	    for (i = 0; ((i < top_page) && (cur_left > 0)); i++) {
	      if ((disks[bestdisk].pages[i / 8] & bit[i % 8]) &&
		  (!dup_segment || ((dup_segment_pages[i / 8] & bit[i % 8]) == 0))) {
		/* Don't move a page if it's in dup_segment */
		printf("Moving page %d from %d (%d left) to %d (%d left).\n",
		       i, bestdisk + 1, left_on_disk(&disks[bestdisk]) + 1,
		       seg0_disk + 1,
		       left_on_disk(&disks[seg0_disk]) - 1);
		cur_left--;
		disks[seg0_disk].pages[i / 8] |= bit[i % 8];
		disks[bestdisk].pages[i / 8] &= ~bit[i % 8];
		break; } }
	    disknum = bestdisk;
	    times_through = 0;
	    continue; }
	  printf("Current algorithm can't fit all segments.\n");
	  display_remaining();
	  exit(1); } }
      else {
	old_remaining = segs_remaining;
	times_through = 0; }
      if (got_some = do_common(seg0_disk)) {
	if (got_some & (1 << last_disk_used))
	  disknum = last_disk_used;
	else if (!(got_some & (1 << disknum))) {
	  int ii;
	  for (ii = seg0_disk + 1; ii < allowed_disks; ii++) {
	    if (got_some & (1 << ii)) {
	      disknum = ii;
	      break; } } } }
      tmp = addnseg(disknum, curseg);
      if (tmp < numsegs) {
	curseg = tmp;
	sts = tmp;
	disks[disknum].last_cur = curseg;
	any_on_disk = 1;
	while ((sts < numsegs) && (left_on_disk(&disks[disknum]) >= target_pages)) {
	  if ((sts = addnaseg(disknum, curseg)) >= numsegs) {
	    if (remaining_adjacent(curseg, 0))
	      break;
	    if ((sts = curseg = newcurseg(disknum, curseg)) >= numsegs)
	      sts = curseg = addnseg(disknum, 0); }
	  if (sts < numsegs)
	    disks[disknum].last_cur = curseg; }
	done = (sts == numsegs); }
      if (!any_on_disk) {
	printf("Current algorithm can't fit all segments.\n");
	display_remaining();
	exit(1); }
      if (!done) {
	if ((allowed_disks - seg0_disk) == 2) {
	  /* There's only one disk beyond preload and segment 0 */
	  if (seg0_disk_used) {
	    printf("Current algorithm can't fit all segments.\n");
	    display_remaining();
	    exit(1); }
	  else {
	    disknum = seg0_disk;
	    seg0_disk_used = 1;
	    continue; } }
	disknum++;
	if (disknum > last_disk)
	  last_disk = disknum;
	if (disknum >= allowed_disks) {
	  last_disk = allowed_disks - 1;
	  /* We've used all the disks we have, and we aren't done */
	  got_some = do_common(seg0_disk);
	  if (got_some && (1 << last_disk_used)) {
	    disknum = last_disk_used;
	    base_any_on_disk = 1;
	    any_on_disk = 1; }
	  else if (got_some && !(got_some & (1 << disknum))) {
	    int ii;
	    for (ii = seg0_disk + 1; ii < allowed_disks; ii++) {
	      if (got_some & (1 << ii)) {
		disknum = ii;
		base_any_on_disk = 1;
		any_on_disk = 1;
		break; } } }
	  else {
	    disknum = seg0_disk + 1;
	    got_some = 1 << disknum;
	    pages_left = 0;
	    base_any_on_disk = 1;
	    for (i = 1; i < numsegs; i++) {
	      if (segments[i].lastsucd == 0) {
		pages_left = pages_left + amt(segments[i].pages); } }
	    target_pages -= pages_left / (allowed_disks - (seg0_disk + 1)); }
	  curseg = disks[disknum].last_cur;
	  while (1) {
	    if ((sts = addnaseg(disknum, curseg)) > numsegs) {
	      if ((sts = curseg = newcurseg(disknum, curseg)) >= numsegs) {
		if (++disknum >= allowed_disks) {
		  if (got_some) {
		    int ii;
		    for (ii = seg0_disk + 1; ii < allowed_disks; ii++) {
		      if (got_some & (1 << ii)) {
			disknum = ii;
			break; } } }
		  else
		    disknum = seg0_disk + 1;
		  break; }
		else
		  curseg = disks[disknum].last_cur; }
	      else if (curseg < numsegs)
		disks[disknum].last_cur = curseg; }
	    else if (sts == numsegs) {
	      if (++disknum >= allowed_disks) {
		if (got_some) {
		  int ii;
		  for (ii = seg0_disk + 1; ii < allowed_disks; ii++) {
		    if (got_some & (1 << ii)) {
		      disknum = ii;
		      break; } } }
		else
		  disknum = seg0_disk + 1;
		break; }
	      else
		curseg = disks[disknum].last_cur; } } }
	else
	  any_on_disk = base_any_on_disk; } } }
  else {
    while((!done) && (disknum < numsegs)) {      /* Start a new disk */
      if ((seg0_disk >= 0) || (addseg0(disknum)) != numsegs + 1) {
	/* Add on segment 0 */
	sts = curseg = addnseg(disknum, 0);         /* Find a new segment */
	if (sts < numsegs)
	  any_on_disk = 1;
	while(sts < numsegs)
	  if ((sts = addnaseg(disknum,curseg)) >= numsegs)  /* Fit adjacent segments */
	    if ((sts = curseg = newcurseg(disknum,curseg)) >= numsegs) 
	      sts = curseg = addnseg(disknum, 0); /* No adjacent; try another new seg */
	done = (sts == numsegs);  /* We are done if there is no new segment to add */
	donefill = FALSE;
	if (any_on_disk) {
	  while (!donefill) {	/* Now fill up the disk */
	    if ((fillseg = nfillseg(disknum)) >= numsegs) /* Get new fillseg */
	      donefill = (addfill(disknum,0) >= numsegs); /* Just write any segment */
	    else
	      if (addfill(disknum,fillseg) >= numsegs) /* Write adjacent segment */ 
		markfill(disknum,fillseg); /* We don't want to use this fillseg again */
	  } }
	else {
	  printf("Current algorithm cannot fit all segments.\n");
	  display_remaining();
	  exit(1); }
      }
      if (!done) {
	any_on_disk = base_any_on_disk;
	disknum++;
	last_disk = disknum; }		/* Go to another disk */
    } }
  if (!done) {
    printf("Current algorithm cannot fit all segments.\n");
    display_remaining();
    exit(1); }
  if ((active_disks == 2) &&
      (left_on_disk(&disks[seg0_disk]) > 0)) {
    do_common(seg0_disk); }
  printf("Split successful.\n");
  return(last_disk+1);
}

display_remaining()
{
  int i;
  for (i = 0; i < numsegs; i++) {
    if (!(segments[i].flags & SF_USED)) {
      printf("Segment %s; %d+%d pages\n", segments[i].name, segments[i].seg_size,
	     topages(segments[i].seg_pic_size)); } } }

/* Add segment 0 to current disk */
addseg0(disknum)
     int disknum;
{
  char *newpages;
  int amount;

  newpages = concat(segments[0].pages,disks[disknum].pages,NULL);
  if ((amount = amt(newpages)) > disks[disknum].free) {
    printf("Segment 0 does not fit on disk %d.\n",disknum + 1);
    return(numsegs + 1);
  }
  copypages(disks[disknum].pages,newpages);
  printf("Adding segment 0 to disk %d; total size now %d.\n",disknum + 1,amount);
  segments[0].flags |= SF_USED;
  return(0);
}


/* Add a new segment to disk. The basic strategy for these segment adding procedures
is to return numsegs if no segment fitting the criteria exists, and numsegs + 1 if
such a segment exists but does not fit. */
addnseg(disknum, curseg)
int disknum;
int curseg;
{
  char *newpages;
  char bpages[PAGENUM / 8];  /* Best set of pages so far */
  int bestseg = numsegs;     /* Best segment to add */
  int bestsize = 0;          /* Best total size */
  int i,amount, tmp = 0, tmp1 = 0;

/* First try to add the startup segment, # 1. If it is unused and it fits, add it. */
  if (((segments[1].flags & SF_USED) == 0) && (segments[1].lastdisk != disknum)) {
    newpages = &bpages[0];
    amount = used_after_segment(&disks[disknum], &segments[1], &newpages);
    if (amount <= disks[disknum].free) {
      bestseg = 1;
      bestsize = amount; }
    else {
      bestseg = numsegs + 1;
      segments[1].lastdisk = disknum; } }
  if (bestseg >= numsegs) { /* Startup not added; look for largest unused segment */
    while (1) {
      if (curseg && (tmp1 = remaining_adjacent(curseg, tmp))) {
	/* There's still stuff adjacent to the current segment. */
	tmp = tmp1;
	newpages = NULL;
	amount = used_after_segment(&disks[disknum], &segments[tmp], &newpages);
	if (amount <= disks[disknum].free) {
	  /* It fits, so do it. */
	  bestsize = amount;
	  bestseg = tmp;
	  copypages(bpages, newpages);
	  break; } }
      else if (tmp) /* There's stuff adjacent to curseg, but it doesn't fit. */
	return(numsegs + 1);
      else break; } }
  if (bestseg >= numsegs) {
    /* Didn't find anything adjacent to curseg. */
    for (i = 2; i < numsegs; i++) {
      if ((segments[i].flags & SF_USED) == 0) {
	newpages = NULL;
	amount = used_after_segment(&disks[disknum], &segments[i], &newpages);
	if (amount <= disks[disknum].free) {
	  if (amount > bestsize) {  /* If its best so far, use it */
	    bestsize = amount;
	    bestseg = i;
	    copypages(bpages,newpages);
	  }
	}
	else {
	  if (bestseg == numsegs)   /* Nothing fit; return numsegs + 1 */
	    bestseg = numsegs + 1;
	  segments[i].lastdisk = disknum;
	}
      } } }
  if (bestseg < numsegs) {
    last_disk_used = disknum;
    segs_remaining--;
    segments[bestseg].lastdisk = segments[bestseg].lastsucd = disknum;
    copypages(disks[disknum].pages,bpages); /* Copy best pages onto disk */
    segments[bestseg].flags |= SF_USED;
    disks[disknum].pix += segments[bestseg].seg_pic_size;
    printf("Adding unused segment %s to disk %d; total size now %d.\n",
	   segments[bestseg].name,disknum + 1,bestsize);
  }
  return(bestseg);
}

/* Return the ID of a segment that's adjacent to curseg and hasn't been used yet. */
remaining_adjacent(curseg, low)
int curseg, low;
{
  int i;
  if (curseg >= numsegs) return(0);
  for (i = low + 1; i < numsegs; i++) {
    if (((segments[i].flags & SF_USED) == 0) &&
	adjacent(curseg, i))
      return(i); }
  return(0); }

/* Add a new adjacent segment. This is basically the same as the above procedure, 
but it also checks for adjacency with the current segment */
addnaseg(disknum,curseg)
     int disknum,curseg;
{
  char *newpages;
  char bpages[PAGENUM / 8];
  int bestseg = numsegs + 1;
  int bestsize = 0;
  int i,amount;

  if (curseg >= numsegs)
    return(bestseg);		/* Can't be anything adjacent */

  for (i = 1; i < numsegs; i++) {
    if (((segments[i].flags & SF_USED) == 0) &&
	(segments[i].lastdisk != disknum) &&
	(adjacent(curseg,i))) {
      newpages = NULL;
      amount = used_after_segment(&disks[disknum], &segments[i], &newpages);
      if (amount <= disks[disknum].free) {
	if (amount > bestsize) {
	  bestsize = amount;
	  bestseg = i;
	  copypages(bpages,newpages); } } } }
  if (bestseg < numsegs) {
    segs_remaining--;
    last_disk_used = disknum;
    segments[bestseg].lastdisk = segments[bestseg].lastsucd = disknum;
    segments[bestseg].flags |= SF_ADJ; /* Both the newly-added seg and curseg have */
    segments[curseg].flags |= SF_ADJ;  /* now been adjacent to another segment */
    copypages(disks[disknum].pages,bpages);
    segments[bestseg].flags |= SF_USED;
    disks[disknum].pix += segments[bestseg].seg_pic_size;
    printf("Adding unused segment %s (adjacent to %s) to disk %d;",
	   segments[bestseg].name,segments[curseg].name,disknum + 1);
    printf(" total size now %d.\n",bestsize);
  }
  return(bestseg);
}


/* Choose a new current segment; a new segment from which to find adjacent
segments. */
newcurseg(disknum,oldcurseg)
     int disknum,oldcurseg;
{
  int i;

  if (oldcurseg < numsegs)
    segments[oldcurseg].lastcurd = disknum;
  /* Mark this one so we won't take it again */
  for (i = 1; i < numsegs; i++)
    if ((segments[i].lastsucd == disknum) && (segments[i].lastcurd != disknum))
      break;
  return(i);
}


/* Same thing, but for when filling disk with used segments. This procedure favors 
segments that are not on a disk with an adjacent segment yet (SF_ADJ not set) */
nfillseg(disknum)
     int disknum;
{
  int i;
  int bestseg = numsegs;

  for (i = 1; i < numsegs; i++)
    if ((segments[i].lastsucd == disknum) &&  /* Written to this disk... */
	(segments[i].lastfild != disknum)) {  /* but not fillseg for this disk */
      bestseg = i;
      if ((segments[i].flags & SF_ADJ) == 0) 
	break;
    }
  return(bestseg);
}


/* Find a used segment to fill disk with. This also favors segments with SF_ADJ
flag not set. If fillseg is 0, it finds any segment.  Otherwise, it finds a 
segment adjacent to fillseg. */
addfill(disknum,fillseg)
     int disknum,fillseg;
{
  char *newpages;
  char bpages[PAGENUM / 8];
  int bestseg = numsegs;
  int bestsize = 0;
  int i,amount;
  char nonadj = FALSE;  /* Whether a seg without SF_ADJ has been found yet */

  for (i = 1; i < numsegs; i++)
    if ((segments[i].lastdisk != disknum) &&  /* Not on this disk yet */
	((fillseg == 0) || (adjacent(fillseg,i))) && /* If need be, adjacent */
	((nonadj) || ((segments[i].flags & SF_ADJ) != 0))) { /* Wants non-SF_ADJ */
      newpages = NULL;
      amount = used_after_segment(&disks[disknum], &segments[i], &newpages);
      if (amount <= disks[disknum].free) {
	if ((amount > bestsize) || ((nonadj) &&  /* Choose best one */
				    ((segments[i].flags & SF_ADJ) != 0))) {
	  nonadj = ((segments[i].flags & SF_ADJ) != 0);
	  bestsize = amount;
	  bestseg = i;
	  copypages(bpages,newpages);
	}
      }
      else {
	if (bestseg == numsegs)
	  bestseg = numsegs + 1;
	segments[i].lastdisk = disknum;
      }
    }
  if (bestseg < numsegs) {
    segments[bestseg].lastdisk = segments[bestseg].lastsucd = disknum;
    if (fillseg != 0) {
      segments[bestseg].flags |= SF_ADJ;
      segments[fillseg].flags |= SF_ADJ;
    }
    copypages(disks[disknum].pages,bpages);
    printf("Filling disk %d with segment %s",disknum + 1,segments[bestseg].name);
    if (fillseg != 0)
      printf(" (adjacent to %s)",segments[fillseg].name);
    disks[disknum].pix += segments[bestseg].seg_pic_size;
    printf("; total size now %d.\n",bestsize);
  } else
    if (fillseg == 0)
      printf("Disk %d is as full as possible.\n",disknum + 1);
  return(bestseg);
}

  
/* Mark a disk as having served as "fillseg" */
markfill(disknum,fillseg)
     int disknum,fillseg;
{
  segments[fillseg].lastfild = disknum;
}


/* Are two segments adjacent? */
adjacent(seg1,seg2)
     int seg1,seg2;
{
  int loc;

  loc = seg1 * numsegs + seg2;
  return(((adjacents[loc / 8] & bit[loc % 8]) != 0));
}


/* Mix two page tables */
char
*concat(table1,table2,dest)
  register long table1[];  /* Treat as longs for speed */
  register long table2[];
  register long dest[];
{
  register int i;
  
  if (dest == NULL)  /* Use default destination if one not given */
    dest = (long *)concatpages;
  for (i = 0; i < PAGENUM / 32; i++)
    dest[i] = table1[i] | table2[i];  /* Concatenate the tables */
  return((char *)dest);
}


delpage(tab,pg)
char *tab;
int pg;
{
  tab[pg / 8] &= ~bit[pg % 8];
}

/* Remove pages of one table from another. */
delpages(tab1,tab2)
     register long tab1[];
     register long tab2[];
{
  register int i;

  for (i = 0; i < PAGENUM / 32; i++)
    tab1[i] &= ~tab2[i];  /* This removes the pages */
}


/* Same as in zsegment.c */
addpages(start,end,table)
           short start;
           short end;
  register char table[];
{
  register int i;

  for (i = start; i <= end; i++)
    table[i / 8] |= bit[i % 8];
}

andpages(dest, src1, src2)
register char *dest;
register char *src1;
register char *src2;
{
  register int i;
  for (i = 0; i < PAGENUM / 8; i++)
    dest[i] = src1[i] & src2[i];
  return(amt(dest)); }

/* Copy pages */
copypages(dest,src)
     register long dest[];
     register long src[];
{
  register int i;

  for (i = 0; i < PAGENUM / 32; i++)
    dest[i] = src[i];
}


/* Return amount of pages in a table */
amt(table)
     register char *table;
{
  register int i;
  register int total = 0;

  for (i = 0; i <= top_page; i++)
    if (table[i / 8] & bit[i % 8])
      total++;
return(total);
}


/* This procedure actually makes the disks. */

makedisks(numdisks,mapest)
     int numdisks;
     int mapest;
{
  int maplen,mappages,i,j;
  int start_lose;
  char *map;

  filename[fnamelen - 4] = NULL;
  strcat(filename,".ds ");        /* filename "template" */
  maplen = mapsize(numdisks);     /* Find out size of map */
  mappages = ((maplen * 2) + PAGESIZE - 1) / PAGESIZE; /* Convert to pages */
  printf("Map size: %d page%s; %d words.\n", mappages, (mappages==1)?"":"s",
	 maplen);
  if (mappages > mapest)
    if (disks[0].free < mappages - mapest) {
      printf("Uh Oh! The disk map doesn't fit. Trying again.");
      free(segments);
      free(disks);
      free(adjacents);
      presplit(mappages,filename);  /* Try again */
      return(0);
    }
  
  map = makemap(maplen,mappages,numdisks);  /* Actually make the map */
  for (i = 0; i < numdisks; i++) {  /* Write the disks */
    filename[fnamelen - 1] = '0' + i + 1;  /* Open the specific disk file */
    if ((dskfd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0666)) < 0) {
      fprintf(stderr,"Could not open disk file %s.\n",filename);
      exit(1);
    }
    printf("Writing disk %d.\n",i + 1);
    if (i == 0) {
      write(dskfd, map, mappages * PAGESIZE);  /* Write the map */
      if (!vpreload) {
	for (j = 0; j <= top_page; j++)  { /* If necessary, write the preload */
	  if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	      ((preload_tab[j / 8] & bit[j % 8]) != 0)) {
	    write(dskfd, objfile + j * PAGESIZE, PAGESIZE);
	    all_pages[j / 8] |= bit[j % 8]; } }
	for (j = 0; j <= top_page; j++) {
	  if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	      ((preload_tab[j / 8] & bit[j % 8]) == 0)) {
	    write(dskfd, objfile + j * PAGESIZE, PAGESIZE);
	    all_pages[j / 8] |= bit[j % 8]; } } } }
    if ((i == 1) && extra_preload) {
      for (j = npre_page; j < (npre_page + extra_preload); j++) {
	/* Dump the extra preload, turn off the bit in the disk map so
	   it won't get seen down below */
	all_pages[j / 8] |= bit[j % 8];
	disks[1].pages[j / 8] &= ~bit[j % 8];
	write(dskfd, objfile + j * PAGESIZE, PAGESIZE); } }
    if ((i != 0) || vpreload) {
      for(j = 0; j <= top_page; j++)   /* Write out the pages in the table */
	if ((disks[i].pages[j / 8] & bit[j % 8]) != 0) {
	  all_pages[j / 8] |= bit[j % 8];
	  write(dskfd,objfile + j * PAGESIZE,PAGESIZE); } }
    close(dskfd);
  }
  start_lose = -1;
  for (j = 0; j <= top_page; j++) {
    if (all_pages[j / 8] & bit[j % 8]) {
      if (start_lose >= 0) {
	if ((start_lose + 1) == j)
	  printf("Page %d not written on any disk.\n", start_lose);
	else
	  printf("Pages %d through %d not written on any disk.\n",
		 start_lose, j - 1);
	start_lose = -1; } }
    else
      start_lose = j; }
  return(0);
}
	

/* Return the size of the disk map */
mapsize(numdisks)
     int numdisks;
{
  int i,j,start,size, first_disk = 0;

  size = MAP_HEADER_SIZE;     /* # of bytes following and # of disks */
  if (preload && funny_disk_0) {
    /* Allow for entries for preload in segment table */
    size += DISK_HEADER_SIZE;
    start = -1;
    first_disk = 1;
    /* Two passes for first disk, since the preload must be contiguous,
       and and may be followed by other stuff that's adjacent to parts
       of it */
    for (j = 0; j <= top_page; j++) {
      if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	  ((preload_tab[j / 8] & bit[j % 8]) != 0)) {
	/* First get the preload */
	if (start == -1) {
	  start = j;
	  size += BLOCK_SIZE; } }
      else
	start = -1; }
    for (j = 0; j <= top_page; j++) {
      if (((disks[0].pages[j / 8] & bit[j % 8]) != 0) &&
	  ((preload_tab[j / 8] & bit[j % 8]) == 0)) {
	if (start == -1) {
	  start = j;
	  size += BLOCK_SIZE; } }
      else
	start = -1; } }
  for (i = first_disk; i < numdisks; i++) {
    size+=DISK_HEADER_SIZE;
    /* For each disk; checksum, picture start, and # of blocks */
    start = -1;
    if (extra_preload && (i == 1)) {
      for (j = 0; j < npre_page; j++) {
	if ((disks[i].pages[j / 8] & bit[j % 8]) != 0) {
	  if (start == -1) {
	    start = j;
	    size += BLOCK_SIZE; } }
	else
	  start = -1; }
      start = -1;
      for (j = npre_page + extra_preload; j <= top_page; j++) {
	if ((disks[i].pages[j / 8] & bit[j % 8]) != 0) {
	  if (start == -1) {
	    start = j;
	    size += BLOCK_SIZE; } }
	else
	  start = -1; }
      size += BLOCK_SIZE;	/* For extra preload block */ }
    else {
      for (j = 0; j <= top_page; j++)
	if (((disks[i].pages[j / 8] & bit[j % 8]) != 0) &&
	    (!extra_preload || (i != 1) ||
	     (j < npre_page) || (j >= (npre_page + extra_preload)))) {
	  if (start == -1) {
	    start = j;
	    size += BLOCK_SIZE;  /* Add three words for a block */
	  }
	} else
	  start = -1;
    } }
  return(size);  /* Convert to bytes */
}



/* Compute checksum for a block */
check(start,end)
     int start,end;
{
  unsigned short total = 0;
  long i;
  for(i = start * PAGESIZE; i < (end + 1) * PAGESIZE; i++)
    total += objfile[i];
  return(total);
}
