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

#ifndef __BSTATUS_PROCESS_H__
#define __BSTATUS_PROCESS_H__


#include <unistd.h>


/*
  Specifies the direction of pipes for process_start, whether the program wants
  to write to the pipe, or read
*/
enum process_direction_t
  {
    PROCESS_DIR_INVALID = 0, // To end the list

    PROCESS_DIR_READ,
    PROCESS_DIR_WRITE,
  };


/*
  Used in order to parse the command line into an argument vector
  If argv is NULL, then calculate the required length of the argument vector,
  excluding the NULL entry
  If argv is not NULL, then parses the command line into *argv, using at most
  *argc elements. Note that NULL characters will be inserted into line to
  seperate the arguments
*/
int
process_parse_argv (char *line, char **argv, int *argc);


/*
  Starts a new process, creating 2 pipes, one for stdin, and one for stdout of
  the child process
  execvp is used to start the new process, using argv[0] as path
  Following argv, untill an integer 0 is given, tuples are taken as an integer
  as the direction of a pipe to create (see process_direction_t above), an
  integer as a file descriptor to bind to in the child, and a pointer to an
  integer, to store the end of the pipe for the program to use.
  The pipes are all set to close on na exec call, and read pipes are set to not
  block
  Returns 0 on error, else the pid of the child
*/
pid_t
process_start (char *argv[], ...);


#endif // __BSTATUS_PROCESS_H__
