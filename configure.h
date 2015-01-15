#ifndef __BSTATUS_CONFIGURE_H__
#define __BSTATUS_CONFIGURE_H__


#include <stdio.h>

#include "item.h"


/*
  Read the command line and returns the config file to use, else NULL on error
  On error, stores the appropriate exit status as *status
*/
const char*
configure_cmd_options (int argc, char *argv[], int *status);


/*
  Read the configuration file given, adding the items to the array given
  Ensure that *item_count and *items are initialized to NULL
  Command line options always override config file options where applicable
  Returns non-zero on success
*/
int
configure_read_file (const char *file_name, int *item_count, item_t ***items);


/*
  Request the delay between interupting and killing children
*/
int
configure_get_kill_delay (void);


#endif
