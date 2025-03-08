/* Resource types specific to zap.  resfile.c Contains general stuff... */

#include <string.h>
#include "zap.h"

#define APPLE_ZPCF_ID 256
#define EGA_ZPCF_ID 257
#define CGA_ZPCF_ID 258
#define AMIGA_ZPCF_ID 259
#define COLOR_MAC_ZPCF_ID 260
#define MAC_ZPCF_ID 261

gen_zpcf_id(machine_name)
char *machine_name;
{
  if (memq("apple", machine_name)) return(APPLE_ZPCF_ID);
  if (memq("ega", machine_name)) return(EGA_ZPCF_ID);
  if (memq("cga", machine_name)) return(CGA_ZPCF_ID);
  if (memq("amiga", machine_name)) return(AMIGA_ZPCF_ID);
  if (memq("mac", machine_name)) {
    if (memq("color", machine_name)) return(COLOR_MAC_ZPCF_ID);
    if (memq("ii", machine_name)) return(COLOR_MAC_ZPCF_ID);
    return(MAC_ZPCF_ID); }
  zerror(E_PASS1, "Unknown machine name in zpcf definition: %s.\n", machine_name);
  return(COLOR_MAC_ZPCF_ID); }

add_zpcf_resource(file_name, machine_name, ideal_x, ideal_y, base_x, base_y)
char *file_name, *machine_name;
int base_x, ideal_x, base_y, ideal_y;
{
  int fnlen = strlen(file_name);
  int mnlen = strlen(machine_name);
  unsigned char *new_data, *tdata;
  int data_len;
  data_len = fnlen + mnlen + 10; /* 2 for strlens, plus four shorts */
  new_data = (unsigned char *)MALLOC(data_len);
  new_data[0] = fnlen;
  memcpy(&new_data[1], file_name, fnlen);
  tdata = &new_data[fnlen + 1];
  tdata[0] = mnlen;
  memcpy(&tdata[1], machine_name, mnlen);
  tdata = &tdata[mnlen + 1];
  stuff_short(tdata, 0, ideal_x);
  stuff_short(tdata, 2, base_x);
  stuff_short(tdata, 4, ideal_y);
  stuff_short(tdata, 6, base_y); /* Now have the resource data */
  add_a_resource("Zpcf", gen_zpcf_id(machine_name), data_len, new_data); }

add_options_resource(do_script, do_ask)
int do_script, do_ask;
{
  unsigned char *opt_data = (unsigned char *)MALLOC(2);
  opt_data[0] = do_script;
  opt_data[1] = do_ask;
  add_a_resource("zopt", 1, 2, opt_data); }
