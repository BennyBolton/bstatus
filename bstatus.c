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
    loop_wait_all (SIGINT, configure_get_kill_delay ());

  loop_wait_all (SIGKILL, 0);

  log_close_all ();

  return exit_status;
}
