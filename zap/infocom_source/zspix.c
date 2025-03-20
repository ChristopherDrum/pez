#include <fcntl.h>
#include <string.h>
#include <memory.h>
#include "splitdefs.h"
#include "zsplitext.h"
#include "splitpic.h"

fill_with_zeroes(fd)
int fd;
{
  char foo = 0;
  long len, nlen;
  lseek(fd, 0L, 2);
  len = lseek(fd, 0L, 1);
  nlen = ((len + PAGESIZE - 1) / PAGESIZE) * PAGESIZE;
  nlen -= len;
  if (nlen) {
    while (nlen-- > 0)
      write(fd, &foo, 1); } }

SEGMENT *find_segment(name)
char *name;
{
  int i = 0;
  while (name[i]) {
    if ((name[i] >= 'a') && (name[i] <= 'z'))
      name[i] = name[i] - 'a' + 'A';
    i++; }
  for (i = 0; i < numsegs; i++) {
    if (strcmp(name, segments[i].name) == 0)
      return(&segments[i]); }
  return(0); }

void load_pic_segs(name)
char *name;
{
  int fd;
  long sz;
  char *buf, *str, *nstr, *nbuf;
  SEGMENT *seg;
  fd = open(name, O_RDONLY);
  if (fd < 0) {
    perror("Can't open picture segment file");
    return; }
  sz = lseek(fd, 0, 2);
  lseek(fd, 0, 0);
  buf = (char *)do_malloc(sz + 1);
  buf[sz] = 0;
  read(fd, buf, sz);
  close(fd);
  nbuf = buf;
  while (1) {
    str = strtok(nbuf, " \n");
    if (!str) break;
    nbuf = (char *)0L;
    nstr = strtok(nbuf, " \n");
    sz = atoi(nstr);
    seg = find_segment(str);
    if (seg)
      seg->seg_pic_size = sz;
    else
      printf("Can't find segment %s.\n", str); }
  /* free(buf); */ }

short allpix[1024];

picinf tpix[500];

void get_picture_data(fn, global)
char *fn;
int *global;
{
  int fd;
  long sz;
  int i, maxpix = 0, npix = 0;
  int segcnt;
  char *buf, *str, *nstr, *nbuf;
  SEGMENT *seg;
  int id, palette, data, len;
  for (i = 0; i < 1024; i++)
    allpix[i] = 0;
  fd = open(fn, O_RDONLY);
  if (fd < 0) {
    perror("Can't open picture detail file");
    return; }
  sz = lseek(fd, 0, 2);
  lseek(fd, 0, 0);
  buf = (char *)do_malloc(sz + 1);
  buf[sz] = 0;
  read(fd, buf, sz);
  close(fd);
  nbuf = buf;
  while (1) {
    str = strtok(nbuf, "\n");
    if (!str) break;
    nbuf = &nbuf[strlen(str) + 1]; /* Point past null that just got added */
    nstr = strtok(str, " ");	/* Get segment name */
    if (!nstr) break;
    seg = find_segment(nstr);
    if (!seg) {
      printf("Can't find segment %s.\n", nstr);
      continue; }
    nstr = strtok(0, " ");
    sz = atoi(nstr);
    seg->seg_pic_size = sz;	/* Now have size of segment pictures in bytes */
    segcnt = 0;
    while (1) {
      nstr = strtok(0, " ");
      if (!nstr) break;
      npix++;
      id = atoi(nstr);
      if (id > maxpix) maxpix = id;
      nstr = strtok(0, " ");
      if (!nstr) {
	printf("Bad detail data (palette) for segment %s.\n", seg->name);
	exit(1); }
      palette = atoi(nstr);
      nstr = strtok(0, " ");
      if (!nstr) {
	printf("Bad detail data (offset) for segment %s.\n", seg->name);
	exit(1); }
      data = atoi(nstr);
      nstr = strtok(0, " ");
      if (!nstr) {
	printf("Bad detail data (length) for segment %s.\n", seg->name);
	exit(1); }
      len = atoi(nstr);
      tpix[segcnt].pi_id = id;
      tpix[segcnt].pi_palette = palette;
      tpix[segcnt].pi_data = data;
      tpix[segcnt].pi_len = len;
      tpix[segcnt].pi_disk_mask = 0;
      segcnt++; }
    if (segcnt > 0) {
      int k;
      seg->seg_picture_count = segcnt;
      seg->seg_picture_info = (picinf *)do_malloc(segcnt * sizeof(picinf));
      memcpy(seg->seg_picture_info, &tpix[0], segcnt * sizeof(picinf)); }
    else
      seg->seg_picture_info = 0; }
/*  free(buf); */
  if (global) {
    *global = (npix * global_entry_size + PAGESIZE - 1) / PAGESIZE;
    printf("Generated global picture directory:  %d pages.\n", *global); } }

short *construct_global_directory()
{
  int i, j, ct = 1;
  SEGMENT *seg;
  picinf *pi;
  for (i = 0; i < 1024; i++)
    allpix[i] = 0;
  for (i = 0; i < numsegs; i++) {
    seg = &segments[i];
    if (!seg->seg_picture_info) continue;
    for (j = 0; j < seg->seg_picture_count; j++) {
      ct++;
      pi = &(seg->seg_picture_info[j]);
      allpix[pi->pi_id] = pi->pi_id | (((seg->seg_disk_mask >> 1) & 0x3f) << 10); } }
  allpix[0] = ct;
  ct = 1;
  for (i = 1; i < 1024; i++) {
    if (allpix[i] != 0) {
      /* Found an entry */
      for (j = ct; j < i; j++) {
	/* Find a zero entry before this to put this guy in.  If there isn't
	   one, we don't have to worry. */
	if (allpix[j] == 0) {
	  allpix[j] = allpix[i];
	  allpix[i] = 0;
	  break; }
	ct = j; } } }
  return(&allpix[0]); }

int words_reversed = 0;

get_word(ptr, which)
unsigned char *ptr;
int which;
{
  if (words_reversed)
    return(ptr[which] | (ptr[which + 1] << 8));
  else
    return((ptr[which] << 8) | ptr[which + 1]); }

dump_word(ptr, which, what)
unsigned char *ptr;
int which, what;
{
  if (words_reversed) {
    ptr[which++] = what & 0377;
    ptr[which] = (what >> 8) & 0377; }
  else {
    ptr[which++] = (what >> 8) & 0377;
    ptr[which] = what & 0377; } }

void dump_triple(ptr, offs, val)
unsigned char *ptr;
int offs, val;
{
  ptr[offs++] = (val >> 16) & 0377;
  ptr[offs++] = (val >> 8) & 0377;
  ptr[offs] = val & 0377; }

int pf_checksum;
int global_count;

unsigned char *insert_picture(old_dir, new_dir, curpic, de_size, local_count)
unsigned char *old_dir, *new_dir;
picinf *curpic;
int de_size, local_count;
{
  int i, j;
  for (i = 0; i < global_count; i++) {
    if (get_word(old_dir, 0) == curpic->pi_id) break;
    old_dir = &old_dir[de_size]; }
  if (i == global_count) {
    printf("Can't find directory entry for picture %d.\n", curpic->pi_id);
    exit(1); }
  for (i = 0; i < local_count; i++) {
    if (get_word(new_dir, 0) > curpic->pi_id) {
      /* Need to open some space.  Possibly overlapping, so do it manually. */
      for (j = (local_count - i) * de_size - 1; j >= 0; j--)
	new_dir[j + de_size] = new_dir[j];
      break; }
    new_dir = &new_dir[de_size]; }
  memcpy(new_dir, old_dir, de_size);
  return(new_dir); }

unsigned char buf[512];

void checksum_write(fd, buf, len)
int fd, len;
unsigned char *buf;
{
  int i;
  for (i = 0; i < len; i++)
    pf_checksum += buf[i];
  write(fd, buf, len); }

do_palette(fd, pfd, curpic, cde, cur_pointer)
int fd, pfd;
picinf *curpic;
unsigned char *cde;
int cur_pointer;
{
  int len, i;
  dump_triple(cde, ld_palette, cur_pointer);
  lseek(pfd, curpic->pi_palette, 0);
  read(pfd, buf, 1);
  len = buf[0];
  write(fd, buf, 1);
  len *= 3;
  cur_pointer += len + 1;
  while (len > 0) {
    if (len < 512) i = len;
    else i = 512;
    read(pfd, buf, i);
    checksum_write(fd, buf, i);
    len -= i; }
  return(cur_pointer); }

do_data(fd, pfd, curpic, cde, cur_pointer, de_size)
int fd, pfd, cur_pointer, de_size;
picinf *curpic;
unsigned char *cde;
{
  int len, i;
  if (de_size == 8)
    dump_triple(cde, apple_ld_data, cur_pointer);
  else
    dump_triple(cde, ld_data, cur_pointer);
  len = curpic->pi_len;
  cur_pointer += len;
  lseek(pfd, curpic->pi_data, 0);
  while (len > 0) {
    if (len > 512) i = 512;
    else i = len;
    read(pfd, buf, i);
    checksum_write(fd, buf, i);
    len -= i; }
  return(cur_pointer); }

unsigned char *old_dir;
int pfd = -1;
int de_size;

output_picture_file(fd, disk_id, pfname)
int fd, disk_id;
char *pfname;
{
  long pic_base, cur_pointer;
  int dir_size;
  int i, j;
  int pic_count = 0;
  int cur_count = 0;
  picinf *segpix, *curpic;
  SEGMENT *seg;
  unsigned char pf_header[header_len];
  unsigned char *dir, *cde;
  if (pfd < 0) {
    pfd = open(pfname, O_RDONLY);
    if (pfd < 0) {
      perror(pfname);
      exit(1); }
    read(pfd, &pf_header[0], header_len);	/* Picture file header */
    de_size = pf_header[hd_entry_size];
    words_reversed = pf_header[hd_flags] & file_flag_words_reversed;
    global_count = get_word(pf_header, hd_local_count);
    old_dir = (unsigned char *)do_malloc(global_count * de_size);
    read(pfd, old_dir, de_size * global_count); }
  pf_header[hd_fid] = disk_id + 1;
  for (i = 0; i < numsegs; i++) {
    seg = &segments[i];
    if (seg->seg_disk_mask & (1 << disk_id))
      pic_count += seg->seg_picture_count; }
  dir_size = pic_count * de_size;
  dir = (unsigned char *)do_calloc(pic_count, de_size);
  pic_base = lseek(fd, 0, 1);
  write(fd, &pf_header[0], header_len);
  write(fd, dir, dir_size);	/* These get re-written later... */
  cur_pointer = pic_base + header_len + dir_size;
  pf_checksum = 0;
  pic_count = 0;
  for (i = 0; i < numsegs; i++) {
    seg = &segments[i];
    if (!(seg->seg_disk_mask & (1 << disk_id)) ||
	(seg->seg_picture_count <= 0)) continue;
    segpix = seg->seg_picture_info;
    for (j = 0; j < seg->seg_picture_count; j++) {
      curpic = &segpix[j];
      cde = insert_picture(old_dir, dir, curpic, de_size, pic_count);
      pic_count++;
      if (curpic->pi_palette)
	cur_pointer = do_palette(fd, pfd, curpic, cde, cur_pointer);
      if (curpic->pi_data)
	cur_pointer = do_data(fd, pfd, curpic, cde, cur_pointer, de_size); } }
  lseek(fd, pic_base + header_len, 0);
  checksum_write(fd, dir, dir_size);
/*  free(dir); */
  lseek(fd, pic_base, 0);
  dump_word(pf_header, hd_local_count, pic_count);
  dump_word(pf_header, hd_checksum, pf_checksum);
  write(fd, &pf_header[0], header_len);
  lseek(fd, 0, 2);
  fill_with_zeroes(fd);
  printf("Added %d pages of pictures to file.  Total size %d.\n",
	 ((cur_pointer - pic_base) + PAGESIZE - 1) / PAGESIZE,
	 (cur_pointer + PAGESIZE - 1) / PAGESIZE);
  return(pf_checksum); }
