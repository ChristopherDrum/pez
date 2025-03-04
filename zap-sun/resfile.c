#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

typedef struct a_resource {
  struct a_resource *res_next;
  int res_id;
  int res_data_len;
  unsigned char *res_data;
  int res_offs; } res;

typedef struct a_res_type {
  long res_type;
  res *next_res;
  int type_count;
  struct a_res_type *next_type; } res_type;

res_type *resources = 0L;
int num_types = 0;
  
/* Write an AppleDouble format resource fork for a file.  As far as we
   can determine, the format is:
0  0x00051607
4  0x00010000
8  Macintosh          (padded to 16 bytes)
24 short, number of entries in list that follows
  26 long, type of first entry in list
	9 is file info, 7 is icon, 2 is resource data, 3 is real file name
  30 long, pointer to data
  34 long, length
  38 next entry ...

128 (say)
  file data
256 (always)
  resource data, as per Inside Mac v. I:
long offset from 256 to resource data
260 long offset from 256 to resource map
264 long length of data
268 long length of map
272 16 bytes FInfo
16 bytes FXinfo
304 4 bytes 0x1fed327, means no map
204 bytes random
*/

#define COUNT_LOC 24
#define ENTRY_LEN 12
#define FIRST_ENTRY 26
#define TYPE_FILE_INFO 9
#define TYPE_DATES 7
#define TYPE_RESOURCE_INFO 2

/* Based on some random date in 1864 that's the basis of times stored
   in the AppleDouble header file */
/* #define JAN_1_1970 030744725620 */
#define JAN_1_1970 030744723624

unsigned char res_buf[512];

res_map(type, length, pointer, which)
long type, length, pointer;
int which;
{
  int where = FIRST_ENTRY + which * ENTRY_LEN;
  res_long(type, where);
  res_long(pointer, where + 4);
  res_long(length, where + 8); }

res_long(num, where)
int where;
long num;
{
  res_buf[where++] = (num >> 24) & 0377;
  res_buf[where++] = (num >> 16) & 0377;
  res_buf[where++] = (num >> 8) & 0377;
  res_buf[where] = num & 0377; }

res_short(num, where)
int where;
short num;
{
  res_buf[where++] = (num >> 8) & 0377;
  res_buf[where] = num & 0377; }

res_string(str, where)
int where;
char *str;
{
  strcpy(&res_buf[where], str);
}

write_a_res(fname, rt, ready, real)
char *fname, *rt;
int ready, real;
{
  int fd;
  int lenloc;
  char new_name[1024];
  char *slash;
  int len;
  int res_offs;
  struct timeval now;
  struct timezone tz;
  unsigned long universal_time;
  gettimeofday(&now, &tz);
  universal_time = (unsigned long)now.tv_sec + (unsigned long)JAN_1_1970;
  strcpy(&new_name[0], fname);
  if (!ready) {
    if (slash = strrchr(&new_name[0], '/'))
      slash = &slash[1];
    else
      slash = &new_name[0];
    len = strlen(slash) + 1;
    while (len--)
      slash[len + 1] = slash[len];
    slash[0] = '%'; }
  fd = open(&new_name[0], O_RDWR | O_TRUNC | O_CREAT, 0666);
  if (fd < 0) {
    printf("Can't write resource file %s.\n", fname);
    return; }
  for (len = 0; len < 511; len++)
    res_buf[len] = 0;
  /* Write the 16-byte header */
  res_long(0x51607L, 0);
  res_long(0x10000L, 4);
  res_string("Macintosh       ", 8);
  /* There are only two entries in the list */
  res_short(3, COUNT_LOC);
  res_map(TYPE_FILE_INFO, 32L, 128L, 0);
  res_map(TYPE_DATES, 16L, 192L, 1);
  res_map(TYPE_RESOURCE_INFO, 0L, 256L, 2);
  res_string(rt, 128);
  res_string("Zdb ", 132);
  res_long(0x1000000, 136);
  res_long(0x00800000, 140);
  res_long(universal_time, 192);
  res_long(universal_time, 196);
  /* All the rest of the file info stuff is zero */
  write(fd, &res_buf[0], 256);
  if (resources && real) {
    /* We have resource stuff to write */
    int res_len = 256, i, res_data_ptr = 0;
    long res_map_starts, type_list_starts, res_data_starts, res_fork_len;
    long tmp;
    short stmp;
    res_type *rt_chain = resources;
    res *cres;
    res_data_starts = lseek(fd, 0L, 1);
    for (i = 0; i < 256; i++)
      res_buf[i] = 0;
    /* We have to write some resource data */
    res_long(256, 0);		/* Beginning of resource data */
    write(fd, &res_buf[0], 256); /* Dump the header.  It's not complete */
    while (rt_chain) {
      /* Write the data, saving the offset to each item */
      cres = rt_chain->next_res;
      while (cres) {
	cres->res_offs = res_data_ptr;
	tmp = cres->res_data_len;
	write(fd, &tmp, 4);
	write(fd, cres->res_data, cres->res_data_len);
	res_data_ptr += 4 + cres->res_data_len;
	cres = cres->res_next; }
      rt_chain = rt_chain->next_type; }
    res_map_starts = lseek(fd, 0L, 1);
    res_long(256 + res_data_ptr, 4); /* Beginning of resource map */
    res_long(res_data_ptr, 8);	/* Length of resource data */
    /* Now write the resource map */
    tmp = 0L;
    write(fd, &tmp, 4);
    write(fd, &tmp, 4);
    write(fd, &tmp, 4);
    write(fd, &tmp, 4);		/* 16 byte--copy of header (fill in later) */
    write(fd, &tmp, 4);		/* handle to next map */
    write(fd, &tmp, 2);		/* file ref number */
    write(fd, &tmp, 2);		/* resource file attributes */
    stmp = 28;
    write(fd, &stmp, 2);	/* Offset from beginning of map to type list */
    stmp = 0;
    write(fd, &stmp, 2);	/* Offset to beginning of name list (fill in later) */
    stmp = num_types - 1;
    write(fd, &stmp, 2);	/* Number of type - 1 */
    res_data_ptr = num_types * 8 + 2; /* Beginning of reference list (relative to
					 beginning of type list */
    rt_chain = resources;
    while (rt_chain) {
      write(fd, &rt_chain->res_type, 4);
      stmp = rt_chain->type_count - 1;
      write(fd, &stmp, 2);
      stmp = res_data_ptr;
      write(fd, &stmp, 2);
      res_data_ptr += 12 * rt_chain->type_count;
      rt_chain = rt_chain->next_type; }
    /* Write the reference lists */
    rt_chain = resources;
    while (rt_chain) {
      cres = rt_chain->next_res;
      while (cres) {
	stmp = cres->res_id;
	write(fd, &stmp, 2);
	stmp = -1;
	write(fd, &stmp, 2);
	tmp = cres->res_offs;
	write(fd, &tmp, 4);
	tmp = 0L;
	write(fd, &tmp, 4);
	cres = cres->res_next; }
      rt_chain = rt_chain->next_type; }
    res_fork_len = lseek(fd, 0L, 1) - res_data_starts; /* Length of resource fork */
    res_long(lseek(fd, 0L, 1) - res_map_starts, 12); /* Length of res map */
    lseek(fd, res_data_starts, 0);
    write(fd, &res_buf[0], 256); /* Now have header */
    lseek(fd, res_map_starts, 0);
    write(fd, &res_buf[0], 16);	/* Copy at beginning of map */
    lseek(fd, res_map_starts + 26, 0);
    stmp = res_fork_len - res_map_starts + res_data_starts;
    write(fd, &stmp, 2);
    lseek(fd, 0L, 0);
    read(fd, &res_buf[0], 256);
    res_map(TYPE_RESOURCE_INFO, res_fork_len, 256L, 2);
    lseek(fd, 0, 0);
    write(fd, &res_buf[0], 256);
    lseek(fd, 0, 2); }
  close(fd); }

stuff_short(buf, offs, what)
unsigned char *buf;
int offs, what;
{
  short *foo = (short *)buf;
  foo[offs / 2] = what; }

res_type *add_a_resource_type(rtype)
char *rtype;
{
  long real_type = (rtype[0] << 24) | (rtype[1] << 16) | (rtype[2] << 8) | rtype[3];
  res_type *rt_chain = resources;
  while (rt_chain) {
    if (rt_chain->res_type == real_type) break;
    rt_chain = rt_chain->next_type; }
  if (!rt_chain) {
    rt_chain = (res_type *)MALLOC(sizeof(res_type));
    rt_chain->res_type = real_type;
    rt_chain->next_res = 0L;
    rt_chain->type_count = 0;
    rt_chain->next_type = resources;
    num_types++;
    resources = rt_chain; }
  return(rt_chain); }

add_a_resource(rtype, id, data_len, data)
char *rtype;
int data_len, id;
unsigned char *data;
{
  res_type *rtype_chain = add_a_resource_type(rtype);
  res *rdata;
  long rtype_long;
  rdata = rtype_chain->next_res;
  while (rdata) {
    if (rdata->res_id == id) {
      printf("Attempt to define two resources of type %s with same id (%d).\n",
	     rtype, id);
      return; }
    rdata = rdata->res_next; }
  rdata = (res *)MALLOC(sizeof(res));
  rdata->res_next = rtype_chain->next_res;
  rtype_chain->next_res = rdata;
  rtype_chain->type_count++;
  rdata->res_data_len = data_len;
  rdata->res_data = data;
  rdata->res_id = id; }

char *memq(str1, str2)
char *str1, *str2;
{
  int len1 = strlen(str1);
  int len2 = strlen(str2);
  int clen1 = len1, clen2 = 0;
  char *cstr1 = str1, *cstr2 = 0;
  char ch, ch1;
  while (clen1 <= len2) {	/* Stop if string we're looking for is too long */
    if (clen1 == 0) return(cstr2);	/* If got all the way to the end, we won */
    ch = cstr1[0];		/* Get next character */
    if ((ch >= 'A') && (ch <= 'Z')) /* We'll check both cases */
      ch1 = ch + 'a' - 'A';
    else if ((ch >= 'a') && (ch <= 'z'))
      ch1 = ch + 'A' - 'a';
    else
      ch1 = ch;
    if ((ch != str2[0]) && (ch1 != str2[0])) {
      cstr1 = str1;		/* If it doesn't match, back to beginning of str1 */
      clen1 = len1;
      if (cstr2) {		/* And back to beginning of match */
	str2 = cstr2;
	len2 = clen2;
	cstr2 = 0; } }
    else {			/* It does match */
      if (!cstr2) {		/* Maybe starting a new one */
	cstr2 = str2;
	clen2 = len2; }
      cstr1++;			/* Advance to next target character */
      clen1--; }
    str2++;
    len2--; }
  return(0); }

add_vers_resource(version, text)
int version;
char *text;
{
  int reslen = 8;		/* Base length */
  char buf[255];
  unsigned char *new_data, *tdata;
  int vlen;
  sprintf(&buf[0], "%d", version);
  vlen = strlen(&buf[0]);
  reslen += vlen;
  sprintf(&buf[0], "%d, %s", version, text);
  reslen += strlen(&buf[0]);
  new_data = (unsigned char *)MALLOC(reslen);
  sprintf(&buf[0], "%d", version);
  new_data[0] = buf[0] - '0';
  if (vlen == 1) new_data[1] = 0;
  else if (vlen == 2) new_data[1] = (buf[1] - '0') << 4;
  else new_data[1] = ((buf[1] - '0') << 4) | (buf[2] - '0');
  new_data[2] = 0x20;
  new_data[3] = 0;
  new_data[4] = 0;			/* Country code? */
  new_data[5] = 0;			/* Rest of country code */
  new_data[6] = vlen;		/* Beginning of PSTRING */
  memcpy(&new_data[7], &buf[0], vlen);
  tdata = &new_data[7 + vlen];
  sprintf(&buf[0], "%d, %s", version, text);
  tdata[0] = vlen = strlen(&buf[0]);
  memcpy(&tdata[1], &buf[0], vlen);
  add_a_resource("vers", 1, reslen, new_data); }
