extern short numsegs;
extern SEGMENT *segments;
extern int allowed_disks;

extern char *do_calloc();
extern SEGMENT *find_segment();
extern void load_pic_segs();
extern void get_picture_data();
extern short *construct_global_directory();
