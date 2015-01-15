#ifndef __BSTATUS_ITEM_H__
#define __BSTATUS_ITEM_H__


typedef struct _item_t item_t;


#include "display.h"

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>



/*
  Structure for each item on the display line, specifying functions to update
  the item text and free an item on close
*/
struct _item_t
{
  char *text;     // Current line of text on display
  int min_length; // Minimum length that text may be, 0 if unspecified

  /*
    Method for updating the item text
    id will be a strictly increasing number, and unique to each call of
    items_update, for drivers that may have multiple entries, but shared data
    See items_update
    The return value of update should be the ammount of milliseconds that the
    item does not want to be updated again untill, or 0 if the item should be
    updated on the next call to items_update
  */
  int (*update) (item_t *item, int id, int delay);

  /*
    Method for handling any events recieved for the item
    event is the event given by display_poll
  */
  void (*event)  (item_t *item, display_event_t *event);

  /*
    Method for clearing/freeing the item
    See items_finish
    if free_item is non-zero, the item should be freed, else the item should
    simply be cleared, so that it can be repurposed, e.g. to display an error
  */
  void (*finish) (item_t *item, int free_item);

  // For storing the line and block passed through (optional)
  char *line;
  char *block;

  /*
    Used by items_update to know when to update the item
    Represents the ammount of milliseconds untill the next update should be
    called, and the ammount delayed thus far respectively
  */
  int wait_time;
  int delayed;
};



typedef struct _item_time_t item_time_t;

/*
  Time structure representing seconds and milliseconds since epoch
*/
struct _item_time_t
{
  long int sec;
  int msec;
};


/*
  Adds a new item to the array items
  Calls realloc on items, so items can be NULL
  if items is reallocated, it is returned, and block should not be freed by the
  caller, else NULL is returned, and the caller should free block itself
  *item_count is incremented if an item is added successfully
*/
item_t**
items_add (int *item_count, item_t **items, const char *line, char *block);


/*
  Calls the update function for all items as neccessary, passing the delay given
  in milliseconds
  Returns the ammount of milliseconds the program should wait untill calling
  items_update again (unless interupted), else 0 if the program should wait
  indefinitely untill interupted
*/
int
items_update (int item_count, item_t **items, int delay);


/*
  Calls finish on all items as neccessary, and frees the items array
*/
void
items_finish (int item_count, item_t **items);


/*
  Allocate and zero a new item, of size given, and set line and block given.
  size must be at least sizeof(item_t)
  item->finish is set to item_finish by this call
  Returns the allocated item on success, else NULL
  Note that if a call to an item driver fails after this call, the item should
  be free'd simply via free rather than item_finish, as the caller will free
  line and block seeing them unused
*/
item_t*
item_alloc (size_t size, char *line, char *block);


/*
  Clear a standard item, by freeing text, line and block if set, and then
  zeroing the item
*/
void
item_finish (item_t *item, int free_item);


/*
  Finishes the item, calling it to deallocate its used resources (short of the
  item itself of course), and turns it into the error message given
*/
void
item_make_error (item_t *item, const char *fmt, ...);


/*
  Same as item_make_error but takes a va_list
*/
void
item_make_error_v (item_t *item, const char *fmt, va_list args);


/*
  Token parser for the text held in items.
  Befour calling this, the caller should set *item to the items text, and
  *colour to COLOUR_DEFAULT
  Returns the next token in the text and updates *token_len and *colour as
  neccessary
  If shorten is not zero, ignores text inside [ ]
  NOTE: the returned token is NOT null terminated, so use *token_len in order to
  display
*/
const char*
item_token_next (const char **item, int *token_len,
                 colour_t *colour, int shorten);


/*
  Return the length in characters of the display line, i.e. the length of
  item->text, excluding control characters and colour information, etc.
 */
size_t
item_current_length (item_t *item, int shorten);



/*
  Get the current time
  If delay is not zero, the timespec is updated by delay millisecs, else is
  reset to current time if the timespec has drifted to far from real time, e.g.
  on init or due to suspend
  Used to sync up operation of the item drivers
  Always returns a valid timespec
*/
item_time_t*
item_get_time (int delay);



/*
  Some standard items
  See items/<file>.c for more information on how each item can be used
*/

item_t*
item_driver_clock (char *line, char *block);

item_t*
item_driver_cpu (char *line, char *block);

item_t*
item_driver_memory (char *line, char *block);

item_t*
item_driver_command (char *line, char *block);


#endif
