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
  Use a script to generate the output line
  Defaultly bstatus-command is used to run this script, see the bstatus-command
  man page for more information on this
  In order to specify an alternative program to run the script, specify its
  argument vector following command on the item line in the configuration file,
  seperating arguments with whitespace (double quoting arguments is allowed)

  The program should output each line that it wants displayed to stdout, and
  can specify the maximum width that the line should take up by starting the
  line with an exclimation mark followed by a integral value

  Events are fed to the program through stdin via the following formats

    click N M - Mouse button N was pressed over the item at position M in the
                string, M may be -1, specifying its unsupported
*/


#include "../item.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "../log.h"
#include "../loop.h"
#include "../process.h"


/*
  Where to find bstatus-command
*/
#define COMMAND_PATH "/usr/bin/bstatus-command"

/*
  Block size to allocate space for command outputs using
*/
#define COMMAND_BUFSIZE 32


typedef struct _command_item_t command_item_t;

/*
  The structure for holding command items
*/
struct _command_item_t
{
  item_t item; // Extend item_t

  int fd_in;  // Child's stdout
  int fd_out; // Child's stdin
  pid_t pid;  // Child's pid

  char *reading;       /* The line currently being read (double buffered with
                          item->text) */
  size_t len;          // The length of the line being read so far
  size_t reading_size; // The size allocated for buffer reading
  size_t text_size;    // The size allocated for item->text
};



/*
  Read any new lines from the process
*/
static int
_item_command_update (item_t *item, int id, int delay)
{
  command_item_t *command = (command_item_t*) item;
  char c;
  char *buf;
  size_t size;
  int retval;

  if (!item)
    return 0;

  if (loop_check_pid (command->pid, &retval) > 0)
    {
      log_error ("_item_command_update: Process ended unexpectedly\n");
      item_make_error (item, "Proccess ended with exit status %d", retval);
      return 0;
    }

  if (!loop_check_fd (command->fd_in))
    return 0;

  while (1)
    {
      retval = read (command->fd_in, &c, 1);

      if (retval == 0) // Stop watching on EOF
        {
          log_error ("_item_command_update: EOF for pipe received\n");
          item_make_error (item, "EOF received");
          break;
        }
      else if (retval < 0) // Stop watching on error
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;

          log_perror ("_item_command_update: read");
          item_make_error (item, "Read error: %s", error_name ());
          return 0;
        }

      if (c == '\n')
        {
          if (command->reading[0] == '!') // !N for min_length
            {
              command->item.min_length = atoi (command->reading + 1);
              command->reading[0] = 0;
            }
          else // Swap the buffers on newline
            {
              buf = command->item.text;
              command->item.text = command->reading;
              command->reading = buf;

              size = command->reading_size;
              command->reading_size = command->text_size;
              command->text_size = size;
            }

          if (command->reading)
            command->reading[0] = 0;
          command->len = 0;
        }
      else // Append the character to command->reading
        {
          if (command->len + 1 >= command->reading_size)
            {
              buf = realloc (command->reading, (command->reading_size
                                                + COMMAND_BUFSIZE));
              if (!buf)
                {
                  log_perror ("_item_command_update: realloc");
                  item_make_error (item, "Realloc error: %s", error_name ());
                  break;
                }

              command->reading = buf;
              command->reading_size += COMMAND_BUFSIZE;
            }

          command->reading[command->len] = c;
          command->len++;
          command->reading[command->len] = 0;
        }
    }

  return 0;
}



/*
  Pass events through to the process
*/
static void
_item_command_event (item_t *item, display_event_t *event)
{
  command_item_t *command = (command_item_t*) item;
  if (!item || !event)
    return;

  switch (event->id)
    {
      case DISPLAY_EVENT_CLICK:
        dprintf (command->fd_out, "click %d %d\n",
                 event->click.button, event->click.position);
        break;
    }
}



/*
  Deallocate all the pipes and buffers and stuffs
*/
static void
_item_command_finish (item_t *item, int free_item)
{
  command_item_t *command = (command_item_t*) item;

  if (command)
    {
      if (command->reading)
        free (command->reading);

      if (command->fd_in >= 0)
        {
          loop_unwatch_fd (command->fd_in);
          close (command->fd_in);
        }

      if (command->fd_out >= 0)
        close (command->fd_out);
    }

  item_finish (item, free_item);
}



item_t*
item_driver_command (char *line, char *block)
{
  int len, free_argv = 1;;
  command_item_t *item;
  char **argv = NULL;

  item = (command_item_t*) item_alloc (sizeof (command_item_t), line, block);
  if (!item)
    return NULL;

  item->item.update = _item_command_update;
  item->item.event  = _item_command_event;
  item->item.finish = _item_command_finish;

  if (line && *line) // Custom program is specified
    {
      if (process_parse_argv (line, NULL, &len))
        {
          argv = malloc (sizeof (char*) * (len + 2));
          if (argv)
            {
              argv[len] = block;
              argv[len + 1] = NULL;

              if (!process_parse_argv (line, argv, &len))
                {
                  free (argv);
                  argv = NULL;
                }
            }
          else
            log_perror ("item_driver_command: malloc");
        }
    }
  else
    {
      argv = (char**) (const char*[]) {COMMAND_PATH, block, NULL};
      free_argv = 0;
    }

  if (!argv)
    {
      free (item);
      return NULL;
    }

  item->pid = process_start (argv,
                             PROCESS_DIR_WRITE, 0, &item->fd_out,
                             PROCESS_DIR_READ,  1, &item->fd_in, 0);
  if (!item->pid)
    {
      free (item);
      item = NULL;
    }
  else
    {
      loop_watch_fd (item->fd_in);
      loop_watch_pid (item->pid);
    }

  if (free_argv)
    free (argv);

  return (item_t*) item;
}
