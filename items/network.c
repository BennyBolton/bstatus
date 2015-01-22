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

/*
  Display information about network utilization
  The line is generated using format_string
  The specifier format is %{d,D,u,U}

    d - Total bytes downloaded on the interface
    D - Current utilization on downloading (in scale of B/s)
    u - Total bytes uploaded on the interface
    U - Current utilization on uploading (in scale of B/s)

  Conditional strings can be specified as
    %{d,D,u,U}{>,<,>=,<=,=,!=}M<string>
  Where M is an integer, and string is a line to display if the comparison is
  matched, e.g. %D>1000000<{ff0000}> would change the colour to red if the
  interface is downloaded at at rate larger than a megabyte per second
*/


#include "../item.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../log.h"
#include "../format.h"



/*
  Prefix to bytes in order of power of 1000
*/
#define NETWORK_PREFIX "kMGTPEZY"



typedef struct _network_item_t network_item_t;

/*
  Structure for holding a network item on the status line
*/
struct _network_item_t
{
  item_t item;

  char *name;              // Interface name
  char *format;            // Format line

  long int bytes_in;       // Bytes received
  long int bytes_in_last;  // Bytes received in the last read
  long int bytes_out;      // Bytes transmitted
  long int bytes_out_last; // Bytes transmitted in the last read

  int delay;               // Time between the last two reads in milliseconds
};



static const char*
network_format_callback (const char **spec, void *data)
{
  static char buf[128];
  const char *str;
  network_item_t *network = (network_item_t*) data;
  long int ammount = -1;
  int speed = 0, remainder, prefix;

  if (!spec || !*spec || !network)
    return NULL;

  *buf = 0;

  switch (**spec)
    {
      case 'd':
        (*spec)++;
        ammount = network->bytes_in;
        break;

      case 'D':
        (*spec)++;
        if (network->delay)
          ammount = ((network->bytes_in - network->bytes_in_last)
                     * 1000 / network->delay);
        else
          ammount = 999999;
        speed = 1;
        break;

      case 'u':
        (*spec)++;
        ammount = network->bytes_out;
        break;

      case 'U':
        (*spec)++;
        if (network->delay)
          ammount = ((network->bytes_out - network->bytes_out_last)
                     * 1000 / network->delay);
        else
          ammount = 999999;
        speed = 1;
        break;
    }

  if (ammount < 0)
    return NULL;

  str = format_read_comparison (spec, ammount);
  if (str)
    return str;

  remainder = 0;
  prefix = -1;

  while (ammount >= 1000 && NETWORK_PREFIX[prefix + 1])
    {
      ammount /= 100;
      remainder = ammount % 10;
      ammount /= 10;
      prefix++;
    }

  if (speed)
    {
      if (prefix >= 0)
        snprintf (buf, sizeof (buf), "%lu.%u %cB/s",
                  ammount, remainder, NETWORK_PREFIX[prefix]);
      else
        snprintf (buf, sizeof (buf), "%lu B/s", ammount);
    }
  else
    {
      if (prefix >= 0)
        snprintf (buf, sizeof (buf), "%lu.%u %cB",
                  ammount, remainder, NETWORK_PREFIX[prefix]);
      else
        snprintf (buf, sizeof (buf), "%lu B", ammount);
    }

  return buf;
}



/*
  States for the automata in network_update
*/
enum item_network_update_state_t
  {
    NETWORK_READING_UNWANTED,
    NETWORK_READING_SPACE,
    NETWORK_READING_IF,
  };



static int
network_update (item_t *item, int id, int delay)
{
  FILE *fp;
  network_item_t *network = (network_item_t*) item;
  int state = NETWORK_READING_UNWANTED, i, c;
  long int down, up;

  if (network)
    {
      network->delay = delay;
      network->bytes_in_last = network->bytes_in;
      network->bytes_out_last = network->bytes_out;
      network->bytes_in = 0;
      network->bytes_out = 0;

      fp = fopen ("/proc/net/dev", "r");
      if (fp)
        {
          while ((c = fgetc (fp)) && c != EOF)
            {
              switch (state)
                {
                  case NETWORK_READING_UNWANTED:
                    if (c == '\n')
                      state = NETWORK_READING_SPACE;
                    break;

                  case NETWORK_READING_SPACE:
                    if (!isspace (c))
                      {
                        if (c == *network->name || *network->name == '*')
                          {
                            i = 1;
                            state = NETWORK_READING_IF;
                          }
                        else
                          state = NETWORK_READING_UNWANTED;
                      }
                    break;

                  case NETWORK_READING_IF:
                    if (c == network->name[i])
                      {
                        i++;
                      }
                    else
                      {
                        if (c == ':' && (network->name[i] == 0
                                         || network->name[0] == '*'))
                          {
                            if (fscanf (fp, " %ld %d %d %d %d %d %d %d %ld",
                                        &down, &i, &i, &i, &i, &i,
                                        &i, &i, &up) == 9)
                              {
                                network->bytes_in  += down;
                                network->bytes_out += up;
                              }
                          }
                        else if (network->name[0] != '*')
                          state = NETWORK_READING_UNWANTED;
                      }
                    break;
                }
            }

          fclose (fp);
        }
      else
        log_perror ("fopen: /proc/net/dev");

      format_string (&item->text, network->format,
                     network_format_callback, network);
    }

  return 500 - (item_get_time (0)->msec % 500);
}



item_t*
item_driver_network (char *line, char *block)
{
  network_item_t *item;

  item = (network_item_t*) item_alloc (sizeof (network_item_t), line, block);
  if (!item)
    return NULL;

  item->item.update = network_update;

  item->name = line;
  item->format = line;

  while (*item->format && !isspace (*item->format))
    item->format++;

  while (isspace (*item->format))
    {
      *item->format = 0;
      item->format++;
    }

  network_update ((item_t*) item, 0, 0);
  item->item.min_length = item_current_length ((item_t*) item, 0);

  return (item_t*) item;
}
