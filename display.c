#include "display.h"

#include <string.h>
#include <ctype.h>

#include "log.h"


/*
  The table of displays used to lookup a display
*/
static struct
{
  const char *name;  // The name of the display
  display_t *driver; // The driver associated
}
  display_table[] =
    {
      {"standard", &display_driver_standard},
      {"i3wm",     &display_driver_i3wm},
      {"command",  &display_driver_command},

      {NULL, NULL} // Mark the end of the array
    };


/*
  The currently set display driver, set by display_set
*/
static display_t *current_display_driver = &display_driver_standard;



display_t*
display_set (const char *name)
{
  int i;
  size_t len;

  if (!name)
    return NULL;

  for (i = 0; display_table[i].name; i++)
    {
      len = strlen (display_table[i].name);
      if (strncmp (name, display_table[i].name, len) == 0
          && (name[len] == 0 || isspace (name[len])))
        {
          if (display_table[i].driver->set)
            if (!display_table[i].driver->set (name + len))
              return NULL;

          if (current_display_driver && current_display_driver->finish)
            current_display_driver->finish (0);

          current_display_driver = display_table[i].driver;

          return display_table[i].driver;
        }
    }

  log_error ("display_set: no such display driver '%s'\n", name);
  return NULL;
}



int
display_start (int item_count)
{
  if (current_display_driver && current_display_driver->start)
    return current_display_driver->start (item_count);

  return 1;
}



void
display_finish (int started)
{
  if (current_display_driver && current_display_driver->finish)
    current_display_driver->finish (started);
}



void
display_update_items (int item_count, item_t *items[])
{
  if (current_display_driver && current_display_driver->update_items)
    current_display_driver->update_items (item_count, items);
}



int
display_poll (display_event_t *event)
{
  if (current_display_driver && current_display_driver->poll)
    return current_display_driver->poll (event);

  return 0;
}
