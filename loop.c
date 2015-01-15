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

#include "loop.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>

#include "log.h"


#define LOOP_MAX_PIDS 64


static fd_set loop_watching_fds; // File descriptors being watched
static fd_set loop_watched_fds;  // File descriptors after select

/*
  Specifies if the program is running
*/
static int loop_running = 1;

/*
  This array is a simple hash table, with hash function x % LOOP_MAX_PIDS, and
  linear probing
  Elements are not removed from the table
*/
static struct
{
  pid_t pid;     // The pid of the process, or 0 for empty
  int8_t dead;   // Non-zero if the process has died
  int8_t status; // Exit status if the process it dead
}
  loop_watched_pids[LOOP_MAX_PIDS];

static int loop_watched_pids_count = 0;
static int loop_watched_pids_alive = 0;



/*
  Catch and process signals
*/
static void
loop_catch_signal (int sig)
{
  switch (sig)
    {
      case SIGALRM:
        break;

      default:
        log_error ("Signal received: %s\nClosing...\n", strsignal (sig));
        loop_running = -1;
        break;
    }
}



void
loop_init (void)
{
  FD_ZERO (&loop_watching_fds);

  signal (SIGINT,  loop_catch_signal);
  signal (SIGTERM, loop_catch_signal);

  signal (SIGALRM, loop_catch_signal);
}



void
loop_watch_fd (int fd)
{
  FD_SET (fd, &loop_watching_fds);
}



void
loop_unwatch_fd (int fd)
{
  FD_CLR (fd, &loop_watching_fds);
}



int
loop_check_fd (int fd)
{
  return FD_ISSET (fd, &loop_watched_fds);
}



/*
  Get the index in loop_watched_pids for the pid.
  If add is non-zero, will alternatively look for the first empty cell if pid is
  not in the table, else will look for the element containing the entry for pid
  Returns -1 on failure
*/
static int
loop_process_index (pid_t pid, int add)
{
  int i;

  i = pid % LOOP_MAX_PIDS;
  while (loop_watched_pids[i].pid)
    {
      if (loop_watched_pids[i].pid == pid)
        return i;

      i = (i + 1) % LOOP_MAX_PIDS;

      if (i == pid % LOOP_MAX_PIDS)
        return -1;
    }

  return add ? i : -1;
}



int
loop_watch_pid (pid_t pid)
{
  int i;

  if (loop_watched_pids_count >= LOOP_MAX_PIDS)
    {
      log_error ("loop_watch_pid: pid watch limit reached\n");
      return 0;
    }

  i = loop_process_index (pid, 1);
  if (i < 0)
    return 0; // Internal error, should never happed

  loop_watched_pids[i].pid = pid;
  loop_watched_pids[i].dead = 0;

  loop_watched_pids_count++;
  loop_watched_pids_alive++;

  log_message ("Watching child process %d\n", pid);

  return 1;
}



int
loop_check_pid (pid_t pid, int *status)
{
  int i;

  i = loop_process_index (pid, 0);

  if (i < 0)
    return -1;

  if (status)
    *status = loop_watched_pids[i].status;

  return loop_watched_pids[i].dead;
}



int
loop_wait_all (int sig, int timeout)
{
  int i, status;
  pid_t pid = 1, sleeping_pid = 0;

  if (sig && loop_watched_pids_alive > 0)
    for (i = 0; i < LOOP_MAX_PIDS; i++)
      if (loop_watched_pids[i].pid && !loop_watched_pids[i].dead)
        kill (loop_watched_pids[i].pid, sig);

  /*
    NOTE: I was originally going to use alarm with this function but it wasn't
    interupting the waitpid call. Didn't find out why, but wasn't to comfortable
    using alarm anyways and I spotted this method online and went for it
  */

  // If a timeout is specified, fork, and have the child sleep, then exit
  if (timeout)
    {
      sleeping_pid = fork ();
      if (sleeping_pid == 0)
        {
          sleep (timeout);
          _exit (0);
        }
      else if (sleeping_pid < 0)
        {
          log_perror ("loop_wait_all: fork");
        }
    }

  while (loop_watched_pids_alive > 0)
    {
      // I make SIGKILL a special cause, to ensure hanging, just to ensure the
      // exit status is read
      pid = waitpid (-1, &status, (timeout || sig == SIGKILL) ? 0 : WNOHANG);

      if (pid < 0 && errno != EINTR)
        {
          log_perror ("loop_wait_all: waitpid");
          return -1;
        }
      else if (pid == 0)
        {
          break;
        }
      else if (pid == sleeping_pid) // timeout has occured
        {
          sleeping_pid = 0;
          break;
        }
      else if (pid > 0)
        {
          log_message ("loop_wait_all: Child process %d ended with status %d\n",
                       (int) pid, WEXITSTATUS (status));

          i = loop_process_index (pid, 0);
          if (i >= 0)
            {
              loop_watched_pids[i].dead = 1;
              loop_watched_pids[i].status = WEXITSTATUS (status);
              loop_watched_pids_alive--;
            }
        }
    }

  if (sleeping_pid > 0) // Kill the timeout process if it is still running
    {
      kill (sleeping_pid, SIGKILL);
      waitpid (sleeping_pid, &i, 0);
    }

  return 0;
}



int
loop_do (int item_count, item_t **items)
{
  struct timeval timeout;
  struct timespec befour, now;
  int wait_time;
  display_event_t event;

  clock_gettime (CLOCK_REALTIME, &befour);

  while (loop_running > 0)
    {
      // Poll the display driver

      while (display_poll (&event))
        {
          switch (event.id)
            {
              case DISPLAY_EVENT_CLICK:
                if (event.click.index >= 0 && event.click.index < item_count
                    && items[event.click.index]->event)
                  items[event.click.index]->event (items[event.click.index],
                                                   &event);
            }
        }

      // Update the items

      clock_gettime (CLOCK_REALTIME, &now);
      timeout.tv_sec = now.tv_sec - befour.tv_sec;
      if (now.tv_nsec < befour.tv_nsec)
        {
          timeout.tv_usec = (now.tv_nsec + 1000000000 - befour.tv_nsec) / 1000;
          timeout.tv_sec--;
        }
      else
        timeout.tv_usec = (now.tv_nsec - befour.tv_nsec) / 1000;

      wait_time = items_update (item_count, items, (timeout.tv_sec * 1000
                                                    + timeout.tv_usec / 1000));

      // Update the display

      display_update_items (item_count, items);

      // update timeval's and call select

      befour.tv_sec = now.tv_sec;
      befour.tv_nsec = now.tv_nsec;

      timeout.tv_sec = 0;
      timeout.tv_usec = wait_time * 1000;

      loop_watched_fds = loop_watching_fds;
      if (select (FD_SETSIZE, &loop_watched_fds,
                  NULL, NULL, wait_time ? &timeout : NULL) < 0
          && errno != EINTR)
        {
          log_perror ("loop_do: select");
          loop_running = -1;
          return -1;
        }

      // Check on the children

      loop_wait_all (0, 0);
    }

  return 0;
}



void
loop_stop (void)
{
  loop_running = 0;
}
