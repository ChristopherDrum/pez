#include <stdio.h>

#ifdef __DATE__

int print_comptime(char *subsys, FILE *output)
{
  if (output)
    return fprintf(output, "%s of %s %s.\n", subsys, __DATE__, __TIME__);
  else
    return printf("%s of %s %s.\n", subsys, __DATE__, __TIME__); }

#else

#ifdef DATE_TIME

int print_comptime(char *subsys, FILE *output)

{
  if (output)
    fprintf(output, "%s of %s.\n", subsys, DATE_TIME);
  else
    printf("%s of %s.\n", subsys, DATE_TIME); }

#else

int print_comptime(char *subsys, FILE *output)

{
  if (output)
    fprintf(output, "%s of unknown date.\n", subsys);
  else
    printf("%s of unknown date.\n", subsys); }

#endif

#endif
