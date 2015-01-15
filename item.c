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

#include "item.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "log.h"



/*
  Prefix to any errors displayed on the status bar
*/
#define ITEM_ERROR_PREFIX "{ff0000}"



/*
  Used by items_update, in order to provide a unique strictly increasing id
*/
static int item_update_count = 0;

/*
  The table of items used to lookup a item handler
*/
static struct
{
  const char *name; // The name of the handler
  item_t *(*driver) (char *line, char *block);
                    // The driver associated
}
  item_table[] =
    {
      {"clock",   item_driver_clock},
      {"command", item_driver_command},
      {"cpu",     item_driver_cpu},
      {"memory",  item_driver_memory},

      {NULL, NULL} // Mark the end of the array
    };



item_t**
items_add (int *item_count, item_t **items, const char *line, char *block)
{
  char *duped_line;
  int i;
  size_t len;
  item_t *item;

  if (!item_count || !line)
    return NULL;

  for (i = 0; item_table[i].name; i++)
    {
      len = strlen (item_table[i].name);

      if (strncmp (line, item_table[i].name, len) == 0
          && (line[len] == 0 || isspace (line[len])))
        {
          line += len;

          while (isspace (line[0]))
            line++;

          break;
        }
    }

  if (!item_table[i].name)
    {
      log_error ("items_add: No such item\n");
      return NULL;
    }

  items = realloc (items, sizeof (item_t*) * ((*item_count) + 1));
  if (!items)
    {
      log_perror ("items_add: realloc");
      return NULL;
    }

  duped_line = strdup (line);
  if (!line)
    {
      log_perror ("items_add: strdup");

      if (block)
        free (block);

      return items;
    }

  item = item_table[i].driver (duped_line, block);
  if (!item)
    {
      log_error ("items_add: %s: Driver returned NULL\n", item_table[i].name);

      if (block)
        free (block);
      free (duped_line);
    }
  else
    {
      items[*item_count] = item;
      (*item_count)++;
    }

  return items;
}



int
items_update (int item_count, item_t **items, int delay)
{
  int i, min_update = 0;

  if (!items)
    return 0;

  item_get_time (delay);

  for (i = 0; i < item_count; i++)
    if (items[i]->update)
      {
        if (items[i]->wait_time <= delay)
          {
            items[i]->wait_time = items[i]->update (items[i], item_update_count,
                                                    delay + items[i]->delayed);
            items[i]->delayed = 0;
          }
        else
          {
            items[i]->wait_time -= delay;
            items[i]->delayed += delay;
          }

        if (min_update == 0
            || (items[i]->wait_time > 0 && items[i]->wait_time < min_update))
          min_update = items[i]->wait_time;
      }

  item_update_count++;

  return min_update;
}



void
items_finish (int item_count, item_t **items)
{
  int i;

  if (!items)
    return;

  for (i = 0; i < item_count; i++)
    if (items[i]->finish)
      items[i]->finish (items[i], 1);

  free (items);
}



item_t*
item_alloc (size_t size, char *line, char *block)
{
  item_t *item;

  if (size < sizeof (item_t))
    {
      log_error ("item_alloc: invalid size given\n");
      return NULL;
    }

  item = malloc (size);
  if (item)
    {
      memset (item, 0, size);
      item->wait_time = 0;
      item->line = line;
      item->block = block;
      item->finish = item_finish;
    }
  else
    log_perror ("item_alloc: malloc");

  return item;
}



void
item_finish (item_t *item, int free_item)
{
  if (item)
    {
      if (item->text)
        free (item->text);

      if (item->line)
        free (item->line);

      if (item->block)
        free (item->block);

      if (free_item)
        free (item);
      else
        memset (item, 0, sizeof (item_t));
    }
}



void
item_make_error (item_t *item, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  item_make_error_v (item, fmt, args);
  va_end (args);
}



void
item_make_error_v (item_t *item, const char *fmt, va_list args)
{
  char *buf;
  int len, plen;
  va_list cpy;

  if (!item)
    return;

  item->finish (item, 0);

  va_copy (cpy, args);

  len = vsnprintf (NULL, 0, fmt, args);
  if (len > 0)
    {
      plen = strlen (ITEM_ERROR_PREFIX);
      buf = malloc (len + 1 + plen);
      if (buf)
        {
          strncpy (buf, ITEM_ERROR_PREFIX, plen);
          buf[plen] = 0;

          vsnprintf (buf + plen, len + 1, fmt, cpy);

          item->text = buf;
          item->finish = item_finish;
        }
      else
        log_perror ("item_make_error: malloc");
    }

  va_end (cpy);
}



const char*
item_token_next (const char **item, int *token_len,
                 colour_t *colour, int shorten)
{
  int escaped = 0;
  const char *token;

  if (!item || !*item || !**item || !token_len)
    return NULL;

  // Read through the control characters

  while (**item && !escaped)
    switch (**item)
      {
        case '[':
          if (shorten)
            {
              while (**item && **item != ']')
                (*item)++;

              if (**item)
                (*item)++;
            }
          else
            (*item)++;

          break;

        case ']':
          (*item)++;
          break;

        case '{':
          (*item)++;

          if (colour)
            {
              if (**item == '}')
                *colour = COLOUR_DEFAULT;
              else
                *colour = strtol (*item, NULL, 16);
            }

          while (**item && **item != '}')
            (*item)++;

          if (**item)
            (*item)++;
          break;

        case '\\':
          (*item)++;
          escaped = 1;
          break;

        default:
          escaped = 1;
          break;
      }

  if (!**item)
    return NULL;

  token = *item;
  *token_len = 0;

  while (**item && (escaped || (**item != '{' && **item != '\\'
                                && **item != '[' && **item != ']')))
    {
      escaped = 0;
      (*item)++;
      (*token_len)++;
    }

  return token;
}



enum item_length_state_t
  {
    ITEM_LENGTH_STATE_TEXT,
    ITEM_LENGTH_STATE_COLOUR,
    ITEM_LENGTH_STATE_ESCAPED,
    ITEM_LENGTH_STATE_OPTIONAL
  };


size_t
item_current_length (item_t *item, int shorten)
{
  size_t len = 0;
  int state = ITEM_LENGTH_STATE_TEXT;
  const char *str;

  if (!item->text)
    return 0;

  str = item->text;

  while (*str)
    {
      switch (state)
        {
          case ITEM_LENGTH_STATE_TEXT:
            switch (*str)
              {
                case '{':
                  state = ITEM_LENGTH_STATE_COLOUR;
                  break;

                case '\\':
                  state = ITEM_LENGTH_STATE_ESCAPED;
                  break;

                case ']':
                  break;

                case '[':
                  if (shorten)
                    state = ITEM_LENGTH_STATE_OPTIONAL;
                  break;

                default:
                  len++;
              }
            break;

          case ITEM_LENGTH_STATE_COLOUR:
            if (*str == '}')
              state = ITEM_LENGTH_STATE_TEXT;
            break;

          case ITEM_LENGTH_STATE_ESCAPED:
            len++;
            state = ITEM_LENGTH_STATE_TEXT;
            break;

          case ITEM_LENGTH_STATE_OPTIONAL:
            if ((*str) == ']')
              state = ITEM_LENGTH_STATE_TEXT;
        }

      str++;
    }

  return len;
}



item_time_t*
item_get_time (int delay)
{
  static item_time_t res = {0};
  struct timespec t;

  if (delay || res.sec == 0)
    {
      res.msec += delay;
      res.sec += (res.msec / 1000);
      res.msec %= 1000;

      clock_gettime (CLOCK_REALTIME, &t);
      if (res.sec - t.tv_sec > 1 || t.tv_sec - res.sec > 1) // Drifted away
        {
          res.sec = t.tv_sec;
          res.msec = t.tv_nsec / 1000000;
        }
    }

  return &res;
}
