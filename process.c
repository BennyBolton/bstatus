#include "process.h"

#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>

#include "log.h"


#define MAX_FDS 6



int
process_parse_argv (char *line, char **argv, int *argc)
{
  int i = 0;

  if (!line || !argc)
    return 0;

  while (*line)
    {
      while (isspace (*line))
        line++;

      if (*line == '"')
        {
          line++;

          if (argv && i < *argc)
            argv[i] = line;

          i++;

          while (*line && *line != '"')
            line++;

          if (*line)
            {
              if (argv)
                *line = 0;

              line++;
            }
        }
      else if (*line)
        {
          if (argv && i < *argc)
            argv[i] = line;

          i++;

          while (*line && !isspace (*line))
            line++;

          if (*line)
            {
              if (argv)
                *line = 0;

              line++;
            }
        }
    }

  *argc = i;

  return 1;
}



pid_t
process_start (char *argv[], ...)
{
  int fds[MAX_FDS];
  int p[2], fd, *store, i, dir;
  int flags, max_fd = 0;
  pid_t pid;
  va_list all, args;

  if (!argv)
    return 0;

  va_start (all, argv);

  // First set all fd's to -1, to mark unused

  va_copy (args, all);

  i = 0;
  while (i < MAX_FDS && va_arg (args, int))
    {
      fd = va_arg (args, int);
      store = va_arg (args, int*);

      if (store && fd >= 0)
        {
          if (fd > max_fd)
            max_fd = fd;

          *store = -1;
          fds[i] = -1;
          i++;
        }
    }

  va_end (args);

  do
    {
      // Open all the pipes

      va_copy (args, all);

      i = 0;
      while (i < MAX_FDS && (dir = va_arg (args, int)))
        {
          fd = va_arg (args, int);
          store = va_arg (args, int*);

          if (!store || fd < 0)
            continue;

          if (pipe (p) < 0)
            {
              i = -1;
              log_perror ("process_start: pipe");
              break;
            }

          if (dir == PROCESS_DIR_READ)
            {
              *store = p[0];
              fds[i] = p[1];
            }
          else
            {
              *store = p[1];
              fds[i] = p[0];
            }

          /*
            Shift the filedescriptor so that it doesn't get closed prematurely
            by a dup2 call
          */
          if (fds[i] != fd && fds[i] <= max_fd) // Unless the fd is already fine
            {
              fd = fcntl (fds[i], F_DUPFD, max_fd + 1);
              if (fd < 0)
                {
                  log_perror ("process_start: fcntl: F_DUPFD");
                  i = -1;
                  break;
                }
              else
                {
                  close (fds[i]);
                  fds[i] = fd;
                }
            }

          flags = fcntl (*store, F_GETFD);
          fcntl (*store, F_SETFD, flags | FD_CLOEXEC);

          if (dir == PROCESS_DIR_READ)
            {
              flags = fcntl (*store, F_GETFL);
              fcntl (*store, F_SETFL, flags | O_NONBLOCK);
            }

          i++;
        }

      if (i < 0)
        break;

      va_end (args);

      // Fork the process

      pid = fork ();
      if (pid < 0) // Error
        {
          log_perror ("process_start: fork");
          break;
        }

      if (pid > 0) // Parent
        {
          // Close all the child end of the pipes

          va_copy (args, all);

          i = 0;
          while (i < MAX_FDS && va_arg (args, int))
            {
              fd = va_arg (args, int);
              store = va_arg (args, int*);

              if (store && fd >= 0)
                {
                  close (fds[i]);
                  i++;
                }
            }

          va_end (args);
          va_end (all);

          return pid;
        }
      // Child

      // dup2 all the pipes

      va_copy (args, all);

      i = 0;
      while (i < MAX_FDS && va_arg (args, int))
        {
          fd = va_arg (args, int);
          store = va_arg (args, int*);

          if (store && fd >= 0)
            {
              dup2 (fds[i], fd);
              i++;
            }
        }

      va_end (args);
      va_end (all);

      execvp (argv[0], argv);
      log_error ("execvp: ");
      log_perror (argv[0]);
      _exit (0);
    }
  while (0);

  // Close opened pipes on error

  va_copy (args, all);

  i = 0;
  while (i < MAX_FDS && va_arg (args, int))
    {
      fd = va_arg (args, int);
      store = va_arg (args, int*);

      if (store && fd >= 0)
        {
          if (*store >= 0)
            {
              close (*store);
              *store = -1;
            }

          if (fds[i] >= 0)
            {
              close (fds[i]);
              fds[i] = -1;
            }

          i++;
        }
    }

  va_end (args);
  va_end (all);

  return 0;
}
