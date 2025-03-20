#include <time.h>
#include <sys/types.h>
#include <sys/times.h>

do_preload_resource(pages)
int pages;
{
  unsigned char *thing = (unsigned char *)do_malloc(1);
  thing[0] = pages;
  add_a_resource("prel", 1, 1, thing); }

do_vers_resource(version, game_name)
int version;
char *game_name;
{
  char buf[255];
  struct tm *tms;
  long tt = time(0);
  tms = localtime(&tt);
  sprintf(&buf[0], "%s for the Apple, %2d %02d/%02d %d:%02d:%02d.",
	  game_name, tms->tm_mon + 1, tms->tm_mday, tms->tm_year,
	  tms->tm_hour, tms->tm_min, tms->tm_sec);
  add_vers_resource(version, &buf[0]); }
