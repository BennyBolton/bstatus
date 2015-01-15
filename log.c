#include "log.h"

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>



/*
  The file streams for logging
*/
static FILE *log_streams[LOG_N_STREAMS] = {NULL};

/*
  The log streams to output to for default locations
*/
static int log_default_streams = LOG_STREAM_ALL;



/*
  Closes the file given if and only if it is not set for another stream
  Note that stream here is the index in log_streams, and can only refer to a
  single stream
*/
static void
log_close_file (int stream)
{
  int i;

  if (stream < 0 || stream >= LOG_N_STREAMS || !log_streams[stream])
    return;

  for (i = 0; i < LOG_N_STREAMS; i++)
    if (i != stream && log_streams[i] == log_streams[stream])
      return;

  fclose (log_streams[stream]);
  log_streams[stream] = NULL;
}



int
log_stream_spec (const char *string, const char **end)
{
  int i, spec = 0, done = 0;

  if (!string)
    return 0;

  while (!done && *string)
    {
      if (*string == 'a')
        spec |= LOG_STREAM_ALL;
      else
        {
          for (i = 0; LOG_STREAM_SPECIFIERS[i]; i++)
            if (*string == LOG_STREAM_SPECIFIERS[i])
              {
                spec |= (1 << i);
                break;
              }

          if (!LOG_STREAM_SPECIFIERS[i])
            break;
        }

      string++;
    }

  if (end)
    *end = string;

  return spec;
}



void
log_suppress (int stream)
{
  log_default_streams &= ~stream;
}



int
log_open_file (const char *url, int stream)
{
  FILE *fp;
  int i;
  time_t t;

  if (!url)
    return 0;

  if ((stream & LOG_STREAM_ALL) == 0) // No appropriate message stream selected
    return 0;

  fp = fopen (url, "ae");
  if (!fp)
    {
      log_perror (url);
      return 0;
    }

  time (&t);
  fprintf (fp, "\n---- %s compiled %s %s\n---- Run %s\n",
           BSTATUS_VERSION, __DATE__, __TIME__, ctime (&t));
  fflush (fp);

  for (i = 0; i < LOG_N_STREAMS; i++)
    if (stream & (1 << i))
      {
        log_close_file (i);
        log_streams[i] = fp;
      }

  return 1;
}



void
log_close_all (void)
{
  int i;

  for (i = 0; i < LOG_N_STREAMS; i++)
    log_close_file (i);
}



void
log_to_stream (int stream, const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  log_to_stream_v (stream, fmt, args);
  va_end (args);
}



void
log_to_stream_v (int stream, const char *fmt, va_list args)
{
  va_list args2;
  int i;

  for (i = 0; i < LOG_N_STREAMS; i++)
    if (stream & (1 << i))
      {
        if (log_default_streams & (1 << i))
          {
            va_copy (args2, args);
            vfprintf (stderr, fmt, args2);
            va_end (args2);
          }

        if (log_streams[i])
          {
            va_copy (args2, args);
            vfprintf (log_streams[i], fmt, args2);
            fflush (log_streams[i]);
            va_end (args2);
          }
      }
}



const char*
error_name ()
{
  return strerror (errno);
}



void
log_perror (const char *name)
{
  int error = errno;

  log_error ("%s: %s\n", name, error_name ());

  errno = error;
}
