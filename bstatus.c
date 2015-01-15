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

#include "configure.h"
#include "display.h"
#include "item.h"
#include "loop.h"
#include "log.h"

#include <stdio.h>
#include <signal.h>



int
main (int argc, char *argv[])
{
  const char *config_file;
  item_t **items = NULL;
  int item_count = 0;
  int exit_status = 0;

  loop_init ();

  config_file = configure_cmd_options (argc, argv, &exit_status);
  if (!config_file)
    return -1;

  if (!configure_read_file (config_file, &item_count, &items))
    {
      exit_status = -1;

      display_finish (0);
    }
  else
    {
      if (display_start (item_count))
        exit_status = loop_do (item_count, items);

      display_finish (1);
    }

  items_finish (item_count, items);

  if (configure_get_kill_delay ())
    loop_wait_all (SIGTERM, configure_get_kill_delay ());

  loop_wait_all (SIGKILL, 0);

  log_close_all ();

  return exit_status;
}
