#include <stdio.h>

#ifdef __DATE__

print_comptime(subsys, output)
char *subsys;
FILE *output;
{
  if (output)
    fprintf(output, "%s of %s %s.\n", subsys, __DATE__, __TIME__);
  else
    printf("%s of %s %s.\n", subsys, __DATE__, __TIME__); }

#else

#ifdef DATE_TIME

print_comptime(subsys, output)
char *subsys;
FILE *output;
{
  if (output)
    fprintf(output, "%s of %s.\n", subsys, DATE_TIME);
  else
    printf("%s of %s.\n", subsys, DATE_TIME); }

#else

print_comptime(subsys, output)
char *subsys;
FILE *output;
{
  if (output)
    fprintf(output, "%s of unknown date.\n", subsys);
  else
    printf("%s of unknown date.\n", subsys); }

#endif

#endif
