/*
  Use an external program for the display driver

  A command line should follow the option in the configuration file
*/


#include "../display.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "../log.h"
#include "../loop.h"
#include "../process.h"



static pid_t command_pid = 0;
static int command_in = -1;
static int command_out = -1;

static char *command_line = NULL;
static char **command_argv = NULL;



int
command_set (const char *line)
{
  int argc;

  command_line = strdup (line);
  if (!command_line)
    {
      log_perror ("command_set: strdup");
      return 0;
    }

  if (!process_parse_argv (command_line, NULL, &argc))
    {
      free (command_line);
      return 0;
    }

  command_argv = malloc (sizeof (char*) * (argc + 1));
  if (!command_argv)
    {
      log_perror ("command_set: malloc");
      free (command_line);
      return 0;
    }

  command_argv[argc] = NULL;

  if (!process_parse_argv (command_line, command_argv, &argc))
    {
      free (command_line);
      free (command_argv);
      return 0;
    }

  return 1;
}



int
command_start (int item_count)
{
  if (command_argv)
    command_pid = process_start (command_argv,
                                 PROCESS_DIR_WRITE, 3, &command_out,
                                 PROCESS_DIR_READ,  4, &command_in, 0);

  if (command_pid)
    {
      loop_watch_fd (command_in);
      loop_watch_pid (command_pid);

      return 1;
    }

  return 0;
}



void
command_finish (int started)
{
  if (command_in >= 0)
    close (command_in);

  if (command_out >= 0)
    close (command_out);

  if (command_line)
    free (command_line);

  if (command_argv)
    free (command_argv);
}



void
command_update_items (int item_count, item_t *items[])
{
  int i;

  if (loop_check_pid (command_pid, &i))
    {
      log_error ("command_update_items: Child %d ended unexpectedly\n",
                 command_pid);

      loop_stop ();
      return;
    }

  for (i = 0; i < item_count; i++)
    {
      if (items[i])
        {
          if (items[i]->text)
            dprintf (command_out, "%d:%s\n",
                     items[i]->min_length, items[i]->text);
          else
            dprintf (command_out, "%d:\n", items[i]->min_length);
        }
    }

  dprintf (command_out, "\n");

  return;
}



enum command_poll_state_t
  {
    STATE_READING_EVENT,

    STATE_READING_CLICK,
    STATE_READING_CLICK_INDEX,
    STATE_READING_CLICK_BUTTON,
    STATE_READING_CLICK_POSITION,

    STATE_READING_INVALID
  };



static int
command_poll (display_event_t *event)
{
  static int state = STATE_READING_EVENT, index, button, position, neg;
  int retval;
  char c;

  if (!event || !loop_check_fd (command_in))
    return 0;

  while (1)
    {
      retval = read (command_in, &c, 1);

      if (retval == 0)
        {
          log_error ("command_poll: Unexpected EOF received\n");
          loop_unwatch_fd (command_in);
          break;
        }

      if (retval < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;

          log_perror ("command_poll: read");
          loop_unwatch_fd (command_in);
          return 0;
        }

      switch (state)
        {
          case STATE_READING_EVENT:
            if (isspace (c))
              continue;

            if (c == 'c')
              state = STATE_READING_CLICK;
            else
              state = STATE_READING_INVALID;
            break;

          case STATE_READING_CLICK:
            if (isspace (c))
              {
                index = button = position = neg = 0;
                state = STATE_READING_CLICK_INDEX;
              }
            break;

          case STATE_READING_CLICK_INDEX:
            if (c == '-')
              neg = 1;
            else if (isspace (c))
              {
                if (neg)
                  index = -index;

                neg = 0;
                state = STATE_READING_CLICK_BUTTON;
              }
            else
              index = index * 10 + c - '0';
            break;

          case STATE_READING_CLICK_BUTTON:
            if (c == '-')
              neg = 1;
            else if (isspace (c))
              {
                if (neg)
                  button = -button;

                neg = 0;
                state = STATE_READING_CLICK_POSITION;
              }
            else
              button = button * 10 + c - '0';
            break;

          case STATE_READING_CLICK_POSITION:
            if (c == '-')
              neg = 1;
            else if (c == '\n')
              {
                if (neg)
                  position = -position;

                state = STATE_READING_EVENT;

                event->id = DISPLAY_EVENT_CLICK;
                event->click.button = button;
                event->click.index = index;
                event->click.position = position;

                return 1;
              }
            else
              position = position * 10 + c - '0';
            break;

          case STATE_READING_INVALID:
            if (c == '\n')
              state = STATE_READING_EVENT;
            break;
        }
    }

  return 0;
}



display_t display_driver_command =
  {
    command_set, command_start, command_finish,
    command_update_items, command_poll
  };
