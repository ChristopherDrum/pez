#include <stdio.h>
#include <time.h>

main(argc, argv)
int argc;
char **argv;
{
  struct tm *ts;
  long clock;
  clock = time(0);
  ts = localtime(&clock);
  printf("cc -g -c comptime.c -D'DATE_TIME=\"%d/%d/%d %d:%02d\"'\n",
	 ts->tm_mon + 1, ts->tm_mday, ts->tm_year % 100,
	 ts->tm_hour, ts->tm_min);
  exit(0); }
