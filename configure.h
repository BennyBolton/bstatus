/*
  This file is part of bstatus.

  bstatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  bstatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with bstatus.  If not, see <http://www.gnu.org/licenses/>.
*/

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
