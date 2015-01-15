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
  This is a really simple program to provide required features to
  bstatus-command not appropriately available in the gnu shell (IMO)

  Just takes an numberic argument (defaulting to 0) and then sleeps for that
  amount of seconds, being interupted on signals, or when stdin has input ready

  All arguments after the first are ignored. If the first argument begins with
  a -, and the following character is not a 1 a help line is displayed

  Exits with a non-zero status if input is not available on stdin
*/

#include <stdlib.h>
#include <stdio.h>

#include <sys/select.h>
#include <sys/time.h>



int main (int argc, char *argv[])
{
  double timeoutf = 0.0f;
  struct timeval timeout;
  fd_set fds;

  FD_ZERO (&fds);
  FD_SET (0, &fds);

  if (argc > 1)
    {
      if (argv[1][0] == '-' && argv[1][1] != '1')
        {
          printf ("Usage: %s <time>\nSleep until standard input is available\n",
                  argv[0]);
          return 0;
        }

      timeoutf = atof (argv[1]);
    }

  timeout.tv_sec = (int) timeoutf;
  timeout.tv_usec = (int) ((timeoutf - (double) timeout.tv_sec) * 1000000);

  if (timeoutf >= 0.0f)
    select (1, &fds, NULL, NULL, &timeout);
  else
    select (1, &fds, NULL, NULL, NULL);

  if (FD_ISSET (0, &fds))
    return 0;
  else
    return 1;
}
