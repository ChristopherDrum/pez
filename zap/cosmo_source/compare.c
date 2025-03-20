/* Compare by Matt Hillman
	Format: compare file1 file2 */

#include <stdio.h>


extern FILE *fopen();


main(argc,argv)
	int argc;
	char **argv;
{
	int f1,f2;
	long loc;
	char *fname1,*fname2;
	FILE *fp1,*fp2;
	
	if (argc < 3) {
		printf("Must supply 2 file names\n");
		exit();
	}
	fname1 = argv[1];
	fname2 = argv[2];
	if (((fp1 = fopen(fname1,"r")) == NULL) ||
		((fp2 = fopen(fname2,"r")) == NULL)) {
		printf("Bad file name!\n");
		exit();
	}
	loc = 0;
	f1 = getc(fp1);
	f2 = getc(fp2);
	while ((f1 != EOF) &&
		   (f2 != EOF)) {
		if (f1 != f2) 
			printf("%ld: %s = %c (%d), %s = %c (%d)\n",loc,fname1,
				   f1,f1,fname2,f2,f2);
		loc++;
		f1 = getc(fp1);
		f2 = getc(fp2);
	}
	if (f1 == EOF)
		printf("%s EOF\n",fname1);
	if (f2 == EOF)
		printf("%s EOF\n",fname2);
}
