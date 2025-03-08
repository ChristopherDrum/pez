#ifndef _SPLITDEFS_
#define _SPLITDEFS_
#define PAGESIZE 512
		/* Bytes per page */
#define PAGENUM 1280
		/* Maximum size of game file */
#define MAP_HEADER_SIZE 10
	/* Size of segment map header (in shorts) */

/* Offsets in header */
#define MH_MAPLEN 0
  	/* Number of words following in map */
#define MH_DISK_COUNT 1
  	/* Number of disks */

#define DISK_HEADER_SIZE 4
  	/* Number of shorts of info for each disk */
#define DH_CHECKSUM 0
#define DH_PICTURE_PAGE 1
  	/* Page in file where picture file starts */
#define DH_BLOCKS 2
  	/* Number of segments for this disk */
#define DH_GLOBAL_PIX 3
  	/* If non-zero, page number of a global picture directory */

#define BLOCK_SIZE 3
#define B_START 0
	/* Starting page number */
#define B_END 1
	/* Ending page number */
#define B_PAGE 2
	/* Disk page of starting page */

typedef struct picinfo {
  int pi_id;
  int pi_palette;
  int pi_data;
  int pi_len;
  int pi_disk_mask; } picinf;

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
    int seg_disk_mask;
    picinf *seg_picture_info;
    int seg_picture_count;
    char pages[PAGENUM / 8];      /* Page bit table */
  } SEGMENT;

#endif

