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

#ifndef __BSTATUS_LOOP_H__
#define __BSTATUS_LOOP_H__


#include "item.h"


/*
  Initialize any resources needed for the main loop
*/
void
loop_init (void);


/*
  Have the main loop watch the file descriptor given as fd
*/
void
loop_watch_fd (int fd);


/*
  Have the main loop no longer watch the file descriptor given as fd
*/
void
loop_unwatch_fd (int fd);


/*
  Check whether the file descriptor given is ready for reading or not
*/
int
loop_check_fd (int fd);


/*
  Have the main loop watch the process with pid given
  Returns non-zero on success, else 0 if the limit on processess watched is
  reached
*/
int
loop_watch_pid (pid_t pid);


/*
  Check whether the process is dead or not
  Returns positive if the process is dead, else 0 except -1 when the process is
  not being watched
  If status is not NULL, will copy the exit status
*/
int
loop_check_pid (pid_t pid, int *status);


/*
  Checks on the processess being watched
  If sig is not zero, first sends sig to all living children.
  If timeout is zero, simply poll the children to see who has
  exited, else waits at most timeout for activity, or untill all children have
  died
 */
int
loop_wait_all (int sig, int timeout);


/*
  Start the main loop, returns the program exit status to use
*/
int
loop_do (int item_count, item_t **items);


/*
  Request the main loop to end
*/
void
loop_stop (void);


#endif // __BSTATUS_LOOP_H__
