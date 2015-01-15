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
  Display memory information
  The line is generated using format_string
  The specifier format is %[p]{m,s,c}{t,f,u,c,a}

    p - If the spec begins with a p, display the value as a percentage of the
        total ammount available in the given group, else format it with the
        appropriate suffix

    m - The value should be about ram
    s - The value should be about swap
    c - The value should be about system commit

    t - Show the total ammount of memory
    f - Show the ammount of memory free
    u - Show the ammount of memory used
    c - Show the ammount of memory cached
    a - Show the ammount of memory available
*/


#include "../item.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../format.h"
#include "../log.h"



/*
  Prefix to bytes in order of power of 1000
*/
#define MEMORY_PREFIX  "kMGTPEZY"



/*
  Structures for storing the current utilization information
*/
static struct _memory_info
{
  size_t total;
  size_t free;
  size_t used;
  size_t cached;
  size_t avail;
}
  ram_info, swap_info, commit_info;


/*
  Bindings of entries in /proc/meminfo to above arrays
*/
static struct _memory_info_binding
{
  const char *name;
  size_t *store;
}
  memory_bindings[] =
    {
      {"MemTotal",     &ram_info.total},
      {"MemFree",      &ram_info.free},
      {"Cached",       &ram_info.cached},
      {"MemAvailable", &ram_info.avail},

      {"SwapTotal",    &swap_info.total},
      {"SwapFree",     &swap_info.free},
      {"SwapCached",   &swap_info.cached},

      {"CommitLimit",  &commit_info.total},
      {"Committed_AS", &commit_info.used},

      {NULL, NULL}
    };



/*
  Recalculate the memory utilization information
*/
static void
_memory_recalculate (int id)
{
  static int prev_id = -1;
  FILE *fp;
  char *line = NULL;
  const char *str;
  size_t n = 0, len;
  int i;

  if (id <= prev_id)
    return;

  prev_id = id;

  fp = fopen ("/proc/meminfo", "r");
  if (!fp)
    return;

  while (getline (&line, &n, fp) >= 0)
    {
      for (i = 0; memory_bindings[i].name; i++)
        {
          len = strlen (memory_bindings[i].name);
          if (strncmp (line, memory_bindings[i].name, len) == 0
              && line[len] == ':')
            {
              str = line + len + 1;
              while (isspace (str[0]))
                str++;

              *memory_bindings[i].store = atol (str);
            }
        }
    }

  if (line)
    free (line);

  fclose (fp);

  // These do not show up directly in /proc/meminfo

  ram_info.used = ram_info.total - ram_info.free;
  swap_info.used = swap_info.total - swap_info.free;
  swap_info.avail = swap_info.free + swap_info.cached;
  commit_info.free = commit_info.total - commit_info.used;
  commit_info.cached = ram_info.cached + swap_info.cached;
  commit_info.avail = commit_info.free;
}



static const char*
_memory_format_callback (const char **spec)
{
  static char buf[32];
  const char *str = NULL;
  struct _memory_info *info;
  long int size;
  int remainder, prefix, as_percent = 0;

  if (!spec || !*spec || !**spec)
    return NULL;

  // p specifies to display the ammount compared to total amount
  if (**spec == 'p')
    {
      as_percent++;
      (*spec)++;
    }

  // Determine what source is requested
  switch (**spec)
    {
      case 'm':
        info = &ram_info;
        break;

      case 's':
        info = &swap_info;
        break;

      case 'c':
        info = &commit_info;
        break;

      default:
        return NULL;
    }
  (*spec)++;

  // Determine which value to display
  switch (**spec)
    {
      case 't':
        size = info->total;
        break;

      case 'f':
        size = info->free;
        break;

      case 'u':
        size = info->used;
        break;

      case 'c':
        size = info->cached;
        break;

      case 'a':
        size = info->avail;
        break;

      default:
        return NULL;
    }
  (*spec)++;

  // Read a comparison if available
  if (as_percent)
    {
      if (info->total > 0)
        str = format_read_comparison (spec, size * 100 / info->total);
    }
  else
    str = format_read_comparison (spec, size);

  if (str)
    return str;

  // Format the value
  if (as_percent)
    {
      if (info->total > 0)
        snprintf (buf, sizeof (buf),
                  "%.1f", size * 100.0f / (float) info->total);
      else
        snprintf (buf, sizeof (buf), "0.0");
    }
  else
    {
      remainder = 0;
      prefix = 0;

      while (size >= 1000 && MEMORY_PREFIX[prefix + 1])
        {
          size /= 100;
          remainder = size % 10;
          size /= 10;
          prefix++;
        }

      if (prefix)
        snprintf (buf, sizeof (buf), "%lu.%u %cB",
                  size, remainder, MEMORY_PREFIX[prefix]);
      else
        snprintf (buf, sizeof (buf), "%lu kB", size);
    }

  return buf;
}



static int
_memory_update (item_t *item, int id, int delay)
{
  _memory_recalculate (id);

  if (item)
    format_string (&item->text, item->line, _memory_format_callback);

  return 500 - (item_get_time (0)->msec % 500);
}



item_t*
item_driver_memory (char *line, char *block)
{
  item_t *item;

  item = item_alloc (sizeof (item_t), line, block);
  if (!item)
    return NULL;

  item->update = _memory_update;

  return item;
}
