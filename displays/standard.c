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
  Generate output strait to a terminal

  The options supported on the display line are as follows:

    align={left,right,center}
      - Specify text alignment within a item

    sep="some string"
      - Specify a string to use for seperating the items
*/


#include "../display.h"

#include <stdio.h>
#include <string.h>

#include "../log.h"
#include "../config.h"


/* TODO: Better coloured output, currently just 8 colours */


#define STANDARD_MAX_SEPARATOR 16

enum standard_alignment_t
  {
    STANDARD_ALIGN_CENTRE,
    STANDARD_ALIGN_LEFT,
    STANDARD_ALIGN_RIGHT
  };

/*
  The separator between items
*/
static char standard_separator[STANDARD_MAX_SEPARATOR] = " | ";

/*
  The alignment method
*/
static int standard_alignment = STANDARD_ALIGN_CENTRE;

/*
  Whether or not to allow coloured output
*/
static int standard_allow_colour = 1;


/*
  Options that can be passed to standard display
*/
static config_option _standard_options[] = {
  {"align",     'a'},
  {"seperator", 's'},
  {"no-color",  'c'},

  {NULL, 0}
};



static int
standard_set (const char *line)
{
  int c, len;
  const char *value;

  while ((c = config_line_next (&line, _standard_options, &value, &len)))
    switch (c)
      {
        case 'a': // Align
          if (len > 0)
            switch (*value)
              {
                case 'l':
                case 'L':
                  standard_alignment = STANDARD_ALIGN_LEFT;
                  break;

                case 'r':
                case 'R':
                  standard_alignment = STANDARD_ALIGN_RIGHT;
                  break;

                case 'c':
                case 'C':
                  standard_alignment = STANDARD_ALIGN_CENTRE;
                  break;
              }
          break;

        case 's': // Separator
          if (len >= STANDARD_MAX_SEPARATOR)
            len = STANDARD_MAX_SEPARATOR - 1;

          strncpy (standard_separator, value, len);
          standard_separator[len] = 0;
          break;

        case 'c': // No-color
          standard_allow_colour = 0;
          break;
      }

  return 1;
}



static void
standard_update_items (int item_count, item_t *items[])
{
  int i, c;
  colour_t colour;
  const char *token, *next;
  int token_len, padding;

  for (i = 0; i < item_count; i++)
    {
      if (i > 0)
        printf ("%s", standard_separator);

      // Calculate ammount of padding neccessary

      if (items[i]->min_length)
        {

          padding = item_current_length (items[i], 0);
          if (padding < items[i]->min_length)
            padding = items[i]->min_length - padding;
          else
            padding = 0;
        }
      else
        padding = 0;

      // Pad to the left

      if (standard_alignment == STANDARD_ALIGN_CENTRE)
        {
          printf ("%*s", padding / 2, "");
          padding = padding - padding / 2;
        }
      else if (standard_alignment == STANDARD_ALIGN_RIGHT)
        {
          printf ("%*s", padding, "");
          padding = 0;
        }

      // Display the item

      next = items[i]->text;
      colour = COLOUR_DEFAULT;

      while ((token = item_token_next (&next, &token_len, &colour, 0)))
        {
          if (standard_allow_colour && colour != COLOUR_DEFAULT)
            {
              c = ((COLOUR_GET_RED (colour) >> 7)
                   | ((COLOUR_GET_GREEN (colour) >> 7) << 1)
                   | ((COLOUR_GET_BLUE (colour) >> 7) << 2));
              printf ("%c[0;3%cm", 033, '0' + c);
            }

          printf ("%.*s", token_len, token);

          if (standard_allow_colour && colour != COLOUR_DEFAULT)
            printf ("%c[0m", 033);
        }

      // Pad to the right

      printf ("%*s", padding, "");
    }

  printf ("\n");
}



display_t display_driver_standard =
  {
    standard_set, NULL, NULL, standard_update_items, NULL
  };
