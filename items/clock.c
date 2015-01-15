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
  A simple clock
  The line is generated using strftime
*/


#include "../item.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../log.h"


#define CLOCK_BUFSIZE 256



static int
clock_update (item_t *item, int id, int delay)
{
  item_time_t *res;
  struct tm *tmp;

  if (!item || !item->text || !item->line)
    return 0;

  res = item_get_time (0);

  tmp = localtime (&res->sec);
  if (tmp == NULL)
    {
      perror ("clock_update: localtime");
      return 0;
    }

  strftime (item->text, CLOCK_BUFSIZE, item->line, tmp);

  return 1000 - res->msec;
}



item_t*
item_driver_clock (char *line, char *block)
{
  item_t *item;

  item = item_alloc (sizeof (item_t), line, block);
  if (!item)
    return NULL;

  item->text = malloc (CLOCK_BUFSIZE);
  if (!item->text)
    {
      log_perror ("item_driver_clock: malloc");
      free (item);
      return NULL;
    }

  item->text[0] = 0;
  item->update = clock_update;
  item->event  = NULL;

  clock_update (item, 0, 0);

  return item;
}
