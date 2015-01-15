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

#ifndef __BSTATUS_DISPLAY_H__
#define __BSTATUS_DISPLAY_H__


#include <stdint.h>


typedef uint32_t colour_t; // Used for coloured output

typedef struct _display_t display_t;

typedef union _display_event_t display_event_t;


#include "item.h"


/*
  Union for holding events
  Currently, not to many events are supported, but more may be added later
*/
union _display_event_t
{
  /*
    Used to identify the type of event, and if neccessary the structure to use
    for the event
  */
  int id;

  /*
    Structure for click events
  */
  struct
  {
    int id; /* Always DISPLAY_EVENT_CLICK */

    int button;   /* The button clicked, i.e. 1-3 for left/middle/right */
    int index;    /* The index of the item clicked as given in display_items */
    int position; /* The index in the item string where the click event occured
                     if supported, else -1 */
  }
    click;
};



/*
  Structure for holding a driver for a display mode, e.g. plain text or ajax
  format driver
*/
struct _display_t
{
  /*
    Is called when the display driver is set, line being any parameters passed.
    Return non-zero if your happy
  */
  int (*set) (const char *line);

  /*
    Method for initializing the display
    See display_start
  */
  int (*start) (int item_count);

  
  /*
    Method for freeing the display
    See display_finish
  */
  void (*finish) (int started);

  /*
    Method for updating the display
    See display_update_items
  */
  void (*update_items) (int item_count, item_t *items[]);

  /*
    Method for polling the display
    See display_poll
  */
  int (*poll) (display_event_t *event);
};



/*
  Sets a display by name
  Returns the display set
  name does not have to be an exact match, just the first word, and then the
  rest of the name is passed to set for the display driver
  Right now the display is simply looked up in a table of standard displays, but
  the possibility exists for supporting plugins or something later on
*/
display_t*
display_set (const char *name);


/*
  This function should initialize the current display driver, and output any
  headers neccessary.
  Returns non-zero on success
*/
int
display_start (int item_count);


/*
  This function should free any resources used by the current display driver,
  and output any trailers neccessary
  started is non-zero if display_start has been called yet, else it is 0
*/
void
display_finish (int started);


/*
  This function updates the display.
  The item string can be read token by token, see item_token_next
*/
void
display_update_items (int item_count, item_t *items[]);


/*
  This function is used to poll the event queue for the display driver
  Returns non-zero while a event is available, setting the event in the pointer
  given
*/
int
display_poll (display_event_t *event);



/*
  Some standard display drivers
*/

/*
  Driver "standard", just to simply output to terminal line after line with
  separator ' | '
*/
extern display_t display_driver_standard;

/*
  Driver "i3wm", for use with i3bar
*/
extern display_t display_driver_i3wm;

/*
  Driver "command", for using an external program as the display driver
*/
extern display_t display_driver_command;



/*
  This macro is used to create a new colour, with red, green and blue expected
  to be integers from 0 to 255 inclusive
*/
#define COLOUR_MAKE(red, green, blue)      \
  (((red) << 16) | ((green) << 8) | (blue))


/*
  The following three macros are used to get the red, green and blue components
  of a colour respectiveley
*/
#define COLOUR_GET_RED(c) \
  (((c) >> 16) & 0xff)

#define COLOUR_GET_GREEN(c) \
  (((c) >> 8) & 0xff)

#define COLOUR_GET_BLUE(c) \
  ((c) & 0xff)


/*
  Some standard colours
*/
#define COLOUR_BLACK 0x000000
#define COLOUR_WHITE 0xffffff

#define COLOUR_RED   0xff0000
#define COLOUR_GREEN 0x00ff00
#define COLOUR_BLUE  0x0000ff

#define COLOUR_YELLOW  0xffff00
#define COLOUR_MAGENTA 0xff00ff
#define COLOUR_CYAN    0x00ffff

/*
  Special out of range colour to specify to use the default appropriate colour
  e.g. white on black or black on white
*/
#define COLOUR_DEFAULT 0x1000000


/*
  Supported events
  More events may be added later
*/
enum display_event_id_t
  {
    /*
      An item has been clicked
      See click sub structure of display_event_t
    */
    DISPLAY_EVENT_CLICK,

    /*
      The display has requested that the items be re-displayed immediately
      No extra information is required for this event
    */
    DISPLAY_EVENT_REFRESH
  };


#endif
