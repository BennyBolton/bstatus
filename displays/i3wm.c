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
  Generate output to be parsed by i3bar, and read input given from i3bar

  The options supported on the display line are as follows:

    align={left,right,center}
      - Specify text alignment within a item

    sep={true,false}
      - Specify whether or not to draw a seperator bar between items

    space=N
      - Specify to space each item by N pixels
*/


#include "../display.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "../config.h"
#include "../loop.h"
#include "../log.h"



/*
  The spacing inbetween items
*/
static int i3wm_spacing = 21;


/*
  Whether or not to draw the separator bar
*/
static const char *i3wm_separator = "true";


/*
  The current alignment
*/
static const char *i3wm_alignment = "center";


/*
  Options that can be passed to i3wm display
*/
static config_option _i3wm_options[] = {
  {"align",     'a'},
  {"seperator", 'b'},
  {"spacing",   's'},

  {NULL, 0}
};



static int
_i3wm_set (const char *line)
{
  int c, len;
  const char *value;

  while ((c = config_line_next (&line, _i3wm_options, &value, &len)))
    switch (c)
      {
        case 'a': // Align
          if (len > 0)
            switch (*value)
              {
                case 'l':
                case 'L':
                  i3wm_alignment = "left";
                  break;

                case 'r':
                case 'R':
                  i3wm_alignment = "right";
                  break;

                case 'c':
                case 'C':
                  i3wm_alignment = "center";
                  break;
              }
          break;

        case 'b': // Separator
          if (len > 0)
            switch (*value)
              {
                case 't':
                case 'T':
                  i3wm_separator = "true";
                  break;

                case 'f':
                case 'F':
                  i3wm_separator = "false";
                  break;
              }
          break;

        case 's': // Spacing
          if (len > 0)
            i3wm_spacing = atol (value);
          break;
      }

  return 1;
}



static int
_i3wm_start (int item_count)
{
  fcntl (0, F_SETFL, O_NONBLOCK);
  loop_watch_fd (0);
  printf ("{ \"version\": 1, \"click_events\": true }\n[\n");

  return 1;
}



static void
_i3wm_finish (int started)
{
  if (started)
    {
      loop_unwatch_fd (0);
      printf ("]\n");
    }
}



static void
_i3wm_update_items (int item_count, item_t *items[])
{
  static int written_yet = 0;

  int i, read_colour;
  colour_t colour;
  const char *token, *next;
  int token_len;

  if (!written_yet)
    {
      printf ("  [");
      written_yet = 1;
    }
  else
    printf (",\n  [");

  for (i = 0; i < item_count; i++)
    {
      if (i > 0)
        printf (",\n    {\n");
      else
        printf ("\n    {\n");

      printf ("      \"name\": \"index_%d\",\n", i);
      printf ("      \"separator\": %s,\n", i3wm_separator);
      printf ("      \"separator_block_width\": %u,\n", i3wm_spacing);

      if (items[i]->min_length)
        {
          printf ("      \"align\": \"%s\",\n", i3wm_alignment);
          printf ("      \"min_width\": \"%*s\",\n",
                  items[i]->min_length, "");
        }

      next = items[i]->text;
      colour = COLOUR_DEFAULT;
      read_colour = 0;

      while ((token = item_token_next (&next, &token_len, &colour, 0)))
        {
          if (!read_colour)
            {
              if (colour != COLOUR_DEFAULT)
                printf ("      \"color\": \"#%06x\",\n", colour);

              printf ("      \"full_text\": \"");
              
              read_colour = 1;
            }

          while (token_len > 0)
            {
              if (*token == '"' || *token == '\\')
                printf ("\\%c", *token);
              else
                printf ("%c", *token);

              token++;
              token_len--;
            }
        }

      if (!read_colour)
        printf ("      \"full_text\": \"(NULL)\",\n");
      else
        printf ("\",\n");

      printf ("      \"short_text\": \"");
      next = items[i]->text;
      read_colour = 0;

      while ((token = item_token_next (&next, &token_len, &colour, 1)))
        {
          read_colour = 1;

          while (token_len > 0)
            {
              if (*token == '"' || *token == '\\')
                printf ("\\%c", *token);
              else
                printf ("%c", *token);

              token++;
              token_len--;
            }
        }

      if (!read_colour)
        printf ("(NULL)");

      printf ("\"\n    }");
    }

  printf ("\n  ]\n");

  fflush (stdout);
}



/*
  States for the automata for reading stdin from i3bar
*/
enum i3wm_poll_automata_state
  {
    STATE_AWAITING_EVENT,
    STATE_AWAITING_NAME,

    STATE_NAME_,
    STATE_NAME_N,
    STATE_NAME_NA,
    STATE_NAME_NAM,
    STATE_NAME_NAME,
    STATE_NAME_B,
    STATE_NAME_BU,
    STATE_NAME_BUT,
    STATE_NAME_BUTT,
    STATE_NAME_BUTTO,
    STATE_NAME_BUTTON,

    STATE_READING_NAME,
    STATE_READING_NAME_PREFIX,
    STATE_READING_NAME_VALUE,

    STATE_READING_BUTTON,
    STATE_READING_BUTTON_VALUE,

    STATE_READING_UNKNOWN_NAME,
    STATE_READING_UNKNOWN_GAP,
    STATE_READING_UNKNOWN_VALUE
  };


static int
_i3wm_poll (display_event_t *event)
{
  char c;
  static int state = STATE_AWAITING_EVENT;
  static int button = 0;
  static int index = -1;
  int retval;

  if (!event || !loop_check_fd (0))
    return 0;

  // This is essentially a DFA
  while (1)
    {
      retval = read (0, &c, 1);

      if (retval == 0)
        {
          log_error ("i3wm_poll: EOF for stdin received\n");
          loop_unwatch_fd (0);
          break;
        }
      else if (retval < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;

          log_perror ("i3wm_poll: read");
          loop_unwatch_fd (0);
          return 0;
        }

      if (c == '}')
        {
          state = STATE_AWAITING_EVENT;
          event->id = DISPLAY_EVENT_CLICK;
          event->click.button = button;
          event->click.index = index;
          event->click.position = -1; // Unsupported

          index = -1;
          button = 0;

          return 1;
        }

      switch (state)
        {
          case STATE_AWAITING_EVENT:
            if (c == '{')
              state = STATE_AWAITING_NAME;
            break;

          case STATE_AWAITING_NAME:
            if (c == '"')
              state = STATE_NAME_;
            break;

          case STATE_NAME_:
            if (c == 'n')
              state = STATE_NAME_N;
            else if (c == 'b')
              state = STATE_NAME_B;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_N:
            if (c == 'a')
              state = STATE_NAME_NA;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_NA:
            if (c == 'm')
              state = STATE_NAME_NAM;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_NAM:
            if (c == 'e')
              state = STATE_NAME_NAME;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_NAME:
            if (c == '"')
              state = STATE_READING_NAME;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_B:
            if (c == 'u')
              state = STATE_NAME_BU;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_BU:
            if (c == 't')
              state = STATE_NAME_BUT;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_BUT:
            if (c == 't')
              state = STATE_NAME_BUTT;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_BUTT:
            if (c == 'o')
              state = STATE_NAME_BUTTO;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_BUTTO:
            if (c == 'n')
              state = STATE_NAME_BUTTON;
            else if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_NAME_BUTTON:
            if (c == '"')
              state = STATE_READING_BUTTON;
            else
              state = STATE_READING_UNKNOWN_NAME;
            break;

          case STATE_READING_NAME:
            if (c == '"')
              state = STATE_READING_NAME_PREFIX;
            else if (c == ',')
              state = STATE_AWAITING_NAME;
            break;

          case STATE_READING_NAME_PREFIX:
            if (c >= '0' && c <= '9')
              {
                index = c - '0';
                state = STATE_READING_NAME_VALUE;
              }
            break;

          case STATE_READING_NAME_VALUE:
            if (c >= '0' && c <= '9')
              {
                index *= 10;
                index += c - '0';
              }
            else if (c == '"')
              state = STATE_AWAITING_NAME;
            break;

          case STATE_READING_BUTTON:
            if (c >= '0' && c <= '9')
              {
                state = STATE_READING_BUTTON_VALUE;
                button = c - '0';
              }
            else if (c == '"')
              state = STATE_READING_UNKNOWN_VALUE;
            break;
            
          case STATE_READING_BUTTON_VALUE:
            if (c >= '0' && c <= '9')
              {
                button *= 10;
                button += c - '0';
              }
            else
              state = STATE_AWAITING_NAME;
            break;

          case STATE_READING_UNKNOWN_NAME:
            if (c == '"')
              state = STATE_READING_UNKNOWN_GAP;
            break;

          case STATE_READING_UNKNOWN_GAP:
            if (c == '"')
              state = STATE_READING_UNKNOWN_VALUE;
            else if (c == ',')
              state = STATE_AWAITING_NAME;
            break;

          case STATE_READING_UNKNOWN_VALUE:
            if (c == '"')
              state = STATE_AWAITING_NAME;
            break;
        }
    }

  return 0;
}



display_t display_driver_i3wm =
  {
    _i3wm_set, _i3wm_start, _i3wm_finish, _i3wm_update_items, _i3wm_poll
  };
