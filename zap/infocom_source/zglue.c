#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <string.h>
#include <memory.h>

#include "splitdefs.h"

#define HEADER_LEN 16
#define HEADER_LOCAL_COUNT 4
#define HEADER_ENTRY_SIZE 8
#define HEADER_CHECKSUM 10
#define APPLE_LD_DATA 5

extern int alphasort();

main(argc, argv)
int argc;
char **argv;
{
  char *game_name = 0;
  char *picture_dir = 0;
  char *machine = "apple";
  char *str;
  print_comptime("zglue", 0);
  while (--argc > 0) {
    str = *++argv;
    if (str[0] == '-') {
      switch (str[1]) {
      case 'g':
	game_name = *++argv;
	argc--;
	break;
      case 'p':
	picture_dir = *++argv;
	argc--;
	break;
      case 'm':
	machine = *++argv;
	argc--;
	break; } }
    else if (game_name == 0)
      game_name = str;
    else if (picture_dir == 0)
      picture_dir = str;
    else
      machine = str; }
  printf("Glue for %s on %s; pictures from %s.\n",
	 game_name, machine, picture_dir);
  glue(game_name, picture_dir, machine); }

/* used by select functions in scandir */
char game[255];
int gn_len;
char pic[255];
int pn_len;

int check_game_file(de)
struct direct *de;
{
  /* Check a directory entry against the game name.  Accept only
     game.dsN, where N is a digit. */
  int i;
  char c;
  if ((de->d_namlen - gn_len) != 1)
    return(0);
  for (i = 0; i < (de->d_namlen - 1); i++) {
    if (game[i] != de->d_name[i])
      return(0); }
  c = de->d_name[de->d_namlen - 1];
  if ((c < '0') || (c > '9'))
    return(0);
  return(1); }

check_pic_file(de)
struct direct *de;
{
  int i;
  char c;
  if ((de->d_namlen - pn_len) != 1)
    return(0);
  for (i = 0; i < (de->d_namlen - 1); i++) {
    if (pic[i] != de->d_name[i])
      return(0); }
  c = de->d_name[de->d_namlen - 1];
  if ((c < '0') || (c > '9'))
    return(0);
  return(1); }

void get_output_name(oname, game_name, file_number)
char *oname, *game_name;
int file_number;
{
  strcpy(oname, game_name);
  strcat(oname, ".dN");
  oname[strlen(oname) - 1] = file_number + '0'; }

struct direct *get_picture_file(picture_files, count, file_number)
struct direct **picture_files;
int file_number, count;
{
  struct direct *pde;
  int j;
  for (j = 0; j < count; j++) {
    pde = picture_files[j];
    if ((pde->d_name[pde->d_namlen - 1] - '0') == file_number)
      return(pde); }
  return(0); }

char file_buf[PAGESIZE];

int copy_picture_file(ofd, ifd, offs)
int ofd, ifd;
long offs;
{
  long got = 0, tmp;
  long save_loc;
  int checksum;
  int entry_size, local_count, i;
  unsigned char fheader[HEADER_LEN];
  unsigned char dir_ent[HEADER_LEN];
  save_loc = lseek(ofd, 0L, 1);
  got += read(ifd, &fheader[0], HEADER_LEN);
  entry_size = fheader[HEADER_ENTRY_SIZE];
  local_count = (fheader[HEADER_LOCAL_COUNT] << 8) | fheader[HEADER_LOCAL_COUNT + 1];
  checksum = (fheader[HEADER_CHECKSUM] << 8) | fheader[HEADER_CHECKSUM + 1];
  write(ofd, &fheader[0], HEADER_LEN);
  for (i = 0; i < local_count; i++) {
    got += read(ifd, &dir_ent[0], entry_size);
    checksum -= (dir_ent[APPLE_LD_DATA] + dir_ent[APPLE_LD_DATA + 1] +
		 dir_ent[APPLE_LD_DATA + 2]);
    tmp = (dir_ent[APPLE_LD_DATA] << 16) | (dir_ent[APPLE_LD_DATA + 1] << 8) |
      dir_ent[APPLE_LD_DATA + 2];
    tmp += offs;
    dir_ent[APPLE_LD_DATA] = (tmp >> 16) & 0xff;
    dir_ent[APPLE_LD_DATA + 1] = (tmp >> 8) & 0xff;
    dir_ent[APPLE_LD_DATA + 2] = tmp & 0xff;
    checksum += dir_ent[APPLE_LD_DATA] + dir_ent[APPLE_LD_DATA + 1] +
      dir_ent[APPLE_LD_DATA + 2];
    write(ofd, &dir_ent[0], entry_size); }
  fheader[HEADER_CHECKSUM] = (checksum >> 8) & 0377;
  fheader[HEADER_CHECKSUM + 1] = checksum & 0377;
  lseek(ofd, save_loc, 0);		/* Back to beginning */
  write(ofd, &fheader[0], HEADER_LEN); /* Rewrite header */
  lseek(ofd, 0, 2);
  while (1) {
    tmp = read(ifd, &file_buf[0], PAGESIZE);
    got += tmp;
    if (tmp == 0) break;
    if (tmp < PAGESIZE) {
      write(ofd, &file_buf[0], tmp);
      break; }
    write(ofd, &file_buf[0], tmp); }
  if (got % PAGESIZE) {
    tmp = PAGESIZE - (got % PAGESIZE);
    for (i = 0; i < tmp; i++)
      file_buf[i] = 0;
    write(ofd, &file_buf[0], tmp);
    got += tmp; }
  return(got / PAGESIZE); }

int copy_file(ofd, ifd)
int ofd, ifd;
{
  int pgcnt = 0;
  int got;
  int do_break = 0;
  while (1) {
    got = read(ifd, &file_buf[0], PAGESIZE);
    if (got == 0) break;
    if (got < PAGESIZE) {
      do_break = 1;
      for (; got < PAGESIZE; got++)
	file_buf[got] = 0; }
    write(ofd, &file_buf[0], got);
    pgcnt++;
    if (do_break) break; }
  return(pgcnt); }

glue(game_name, picture_dir, machine)
char *game_name, *picture_dir, *machine;
{
  struct direct **picture_files;
  struct direct **game_files;
  struct direct *gde;
  struct direct *pde;
  char oname[255];
  int fd0, ifd, ofd, pgcnt, ppgcnt;
  int file_number;
  int i;
  int game_count, picture_count;
  unsigned short *header, *disk_ent;
  short header_len, header_bytes;
  strcpy(&game[0], game_name);
  strcat(&game[0], ".ds");
  gn_len = strlen(&game[0]);
  strcpy(&pic[0], game_name);
  strcat(&pic[0], ".");
  strcat(&pic[0], machine);
  strcat(&pic[0], ".");
  pn_len = strlen(&pic[0]);
  if ((game_count = scandir(".", &game_files, check_game_file, alphasort)) <= 0) {
    printf("Can't find any game files for %s.\n", game_name);
    exit(1); }
  if ((picture_count = scandir(picture_dir, &picture_files, check_pic_file, alphasort))
      <= 0) {
    printf("Can't find any picture files for %s %s in %s.\n",
	   machine, game_name, picture_dir);
    exit(1); }
  /* Now have lists of the game segments and picture files */
  if (game_files[0]->d_name[game_files[0]->d_namlen - 1] != '1') {
    printf("No %s.ds1 file found.\n", game_name);
    exit(1); }
  fd0 = open(game_files[0]->d_name, O_RDONLY);
  if (fd0 < 0) {
    perror("Can't open preload file");
    exit(1); }
  read(fd0, &header_len, sizeof(short));
  header_len++;			/* Length word doesn't include itself */
  header_bytes = ((header_len * sizeof(short) + (PAGESIZE - 1)) / PAGESIZE) *
    PAGESIZE;			/* Turn into pages */
  header = (unsigned short *)malloc(header_bytes);
  read(fd0, &header[1], header_bytes - sizeof(short)); /* Read it in */
  header[MH_MAPLEN] = header_len - 1;
  if (header[MH_DISK_COUNT] != game_count) {
    printf("Found %d disks, but header wants %d.\n", game_count,
	   header[MH_DISK_COUNT]);
    exit(1); }
  if (header[MH_DISK_COUNT] < picture_count) {
    printf("Found %d picture files, but only %d disks.\n", picture_count,
	   header[MH_DISK_COUNT]);
    exit(1); }
  pde = 0;
  disk_ent = &header[MAP_HEADER_SIZE];
  /* Entry for disk 1 */
  disk_ent = &disk_ent[DISK_HEADER_SIZE + disk_ent[DH_BLOCKS] * BLOCK_SIZE];
  /* Now have next disk */
  for (i = 2; i <= game_count; i++) {
    gde = game_files[i - 1];
    if ((ifd = open(gde->d_name, O_RDONLY)) < 0) {
      perror(gde->d_name);
      exit(1); }
    file_number = gde->d_name[gde->d_namlen - 1] - '0';
    get_output_name(&oname[0], game_name, file_number);
    if ((ofd = open(&oname[0], O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
      perror(&oname[0]);
      exit(1); }
    pgcnt = copy_file(ofd, ifd);
    if (disk_ent[DH_GLOBAL_PIX] != 0) {
      char gname[255];
      unsigned char buf[PAGESIZE];
      int gfd, got;
      int checksum = disk_ent[DH_CHECKSUM];
      strcpy(&gname[0], picture_dir);
      strcat(&gname[0], "/");
      strcat(&gname[0], game_name);
      strcat(&gname[0], ".");
      strcat(&gname[0], machine);
      strcat(&gname[0], ".G");
      gfd = open(&gname[0], O_RDONLY);
      if (gfd >= 0) {
	int gcnt = 0, k;
	disk_ent[DH_GLOBAL_PIX] = pgcnt;
	while (1) {
	  int do_break = 0;
	  got = read(gfd, &buf[0], PAGESIZE);
	  if (got == 0) break;
	  if (got < PAGESIZE) {
	    do_break = 1;
	    for (; got < PAGESIZE; got++)
	      buf[got] = 0; }
	  for (k = 0; k < PAGESIZE; k++)
	    checksum += buf[k];
	  write(ofd, &buf[0], got);
	  pgcnt++;
	  gcnt++;
	  if (do_break) break; }
	disk_ent[DH_CHECKSUM] = checksum;
	printf("%s: global picture directory size is %d.\n", &oname[0], gcnt);
	close(gfd); }
      else {
	perror(&gname[0]);
	exit(1); } }
    pde = get_picture_file(picture_files, picture_count, file_number);
    if (!pde) {
      disk_ent[DH_PICTURE_PAGE] = 0;
      printf("%s: %d data pages.\n", &oname[0], pgcnt); }
    else {
      strcpy(&pic[0], picture_dir);
      strcat(&pic[0], "/");
      strcat(&pic[0], pde->d_name);
      close(ifd);
      if ((ifd = open(&pic[0], O_RDONLY)) < 0) {
	perror(pde->d_name);
	exit(1); }
      disk_ent[DH_PICTURE_PAGE] = pgcnt;
      ppgcnt = copy_picture_file(ofd, ifd, pgcnt * PAGESIZE);
      printf("%s: %d data pages, %d picture pages.\n", &oname[0], pgcnt, ppgcnt); }
    close(ifd);
    close(ofd);
    disk_ent = &disk_ent[DISK_HEADER_SIZE + disk_ent[DH_BLOCKS] * BLOCK_SIZE]; }
  get_output_name(&oname[0], game_name, 1);
  if ((ofd = open(&oname[0], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
    perror(&oname[0]);
    exit(1); }
  header[MAP_HEADER_SIZE + DH_PICTURE_PAGE] = 0;
  write(ofd, header, header_bytes);
  pgcnt = copy_file(ofd, fd0);
  printf("%s: %d data pages, %d (%d) bytes of header.\n", &oname[0], pgcnt,
	 (header_len + 1) * sizeof(short), header_bytes);
  close(ofd);
  close(fd0); }
