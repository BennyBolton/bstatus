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

#ifndef __BSTATUS_LOG_H__
#define __BSTATUS_LOG_H__


#include <stdarg.h>


/*
  Used to specify which streams to affect with below functions where applicable
*/
#define LOG_STREAM_MESSAGE 1
#define LOG_STREAM_ERROR   2

#define LOG_STREAM_ALL     3

/*
  The ammount of streams available
*/
#define LOG_N_STREAMS      2

/*
  The specifier characters for the available log streams, in order
  In addition to these, 'a' is used to specify all
*/
#define LOG_STREAM_SPECIFIERS "me"



/*
  Read a line for log stream specifiers
  If end is not NULL, the location of the first invalid character is stored as
  *end
  Log stream specifiers available:
    m - Message stream
    e - Error stream
    a - All streams, equivalent to 'me'
*/
int
log_stream_spec (const char *string, const char **end);


/*
  Suppresses outputing to the default locations on the specified streams
  Does not effect file logging via log_open_file
*/
void
log_suppress (int stream);


/*
  Open a stream for outputing messages and/or errors
*/
int
log_open_file (const char *url, int stream);


/*
  Closes all open log files
*/
void
log_close_all (void);


/*
  Log to the appropriate facilites for the specified streams
*/
void
log_to_stream (int stream, const char *fmt, ...);


/*
  Same as log_to_stream, but takes a va_list
*/
void
log_to_stream_v (int stream, const char *fmt, va_list args);


/*
  Macros for all streams
*/

#define log_message(...) log_to_stream (LOG_STREAM_MESSAGE, __VA_ARGS__)

#define log_message_v(fmt, args) log_to_stream (LOG_STREAM_MESSAGE, fmt, args)

#define log_error(...) log_to_stream (LOG_STREAM_ERROR, __VA_ARGS__)

#define log_error_v(fmt, args) log_to_stream_v (LOG_STREAM_ERROR, fmt, args)


/*
  Helper function to get the current error as a string
  I figured it might be easier this way, rather than including string.h and
  errno.h in any file wanting to log an error
*/
const char*
error_name ();


/*
  Similar to perror, but using log_error
  Ensures that errno is not altered
*/
void
log_perror (const char *name);


#endif
