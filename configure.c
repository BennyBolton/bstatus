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

#include "configure.h"

#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "config.h"
#include "log.h"



/*
  Options available in the configuration file
  See the man page for bstatus.conf for more information
*/
static config_option config_file_options[] =
  {
    {"display",    'd'},
    {"item",       'i'},
    {"log",        'l'},
    {"suppress",   's'},
    {"kill-delay", 'k'},

    {0, 0}
  };


/*
  Options available over command line
  See the man page for bstatus for more information
*/
static struct option config_line_options[] =
  {
    {"config",     required_argument, NULL, 'c'},
    {"log",        required_argument, NULL, 'l'},
    {"suppress",   required_argument, NULL, 's'},
    {"kill-delay", required_argument, NULL, 'k'},

    {"help",       no_argument,       NULL, 'h'},
    {"version",    no_argument,       NULL, 'v'},

    {0, 0, 0, 0}
  };



/*
  Bit flags for specifying which options have been set via command line
*/
#define CONFIGURE_KILL_DELAY 1


/*
  Holds which items have been set via command line so to avaid them in config
  file
*/
static int configure_set_options = 0;

/*
  Similarily to configure_set_options, specifies which streams log files have
  been specified for
*/
static int configure_set_log_files = 0;


/*
  The current set kill delay, see configure_get_kill_delay
*/
static int configure_kill_delay = 5;


/*
  Symbols set by objcopy for reading help.txt
*/
extern char _binary_help_txt_start;
extern char _binary_help_txt_end;



/*
  Quickly check if the file given exists
  Returns non-zero if it does
*/
static int
_check_file_exists (const char *filename)
{
  struct stat s;

  if (stat (filename, &s) != 0)
    {
      log_perror (filename);
      return 0;
    }

  return S_ISREG (s.st_mode);
}



const char*
configure_cmd_options (int argc, char *argv[], int *status)
{
  int c;
  const char *str;
  static char config_file[FILENAME_MAX] = {0};

  if (!argv || !status)
    return NULL;

  while (1)
    {
      c = getopt_long (argc, argv, "hvc:l:s:k:", config_line_options, NULL);

      if (c < 0)
        break;

      switch (c)
        {
          case 'c': // Config
            strncpy (config_file, optarg, FILENAME_MAX);
            config_file[FILENAME_MAX - 1] = 0;
            break;

          case 'l': // Log
            c = log_stream_spec (optarg, &str);
            if (!str || *str != ':')
              {
                log_error ("-l: Expected filename\n");
                *status = -1;
                return NULL;
              }

            configure_set_log_files |= c;
            log_open_file (str + 1, c);
            break;

          case 's': // Suppress
            c = log_stream_spec (optarg, NULL);
            log_suppress (c);
            break;

          case 'k': // Kill delay
            configure_set_options |= CONFIGURE_KILL_DELAY;
            configure_kill_delay = atoi (optarg);
            break;

          case 'v': // Version
            printf ("%s\n", BSTATUS_VERSION);
            *status = 0;
            return NULL;
            break;

          default: // Help or error
            printf ("%.*s",
                    (int) (&_binary_help_txt_end - &_binary_help_txt_start),
                    &_binary_help_txt_start);
            *status = (c == 'h') ? 0 : -1;
            return NULL;
        }
    }

  if (*config_file)
    return config_file;

  // Default the config file to ~/.bstatus.conf

  str = getenv ("HOME");
  if (str)
    {
      strncpy (config_file, str, FILENAME_MAX);
      config_file[FILENAME_MAX - 1] = 0;

      strncat (config_file, "/.bstatus.conf",
               FILENAME_MAX - strlen (config_file) - 1);
    }

  // If ~/.bstatus.conf doesn't exist, use /etc/bstatus.conf

  if (!*config_file || !_check_file_exists (config_file))
    {
      strncpy (config_file, "/etc/bstatus.conf", FILENAME_MAX);
      config_file[FILENAME_MAX - 1] = 0;
    }

  return config_file;
}



int
configure_read_file (const char *filename, int *item_count, item_t ***items)
{
  const char *line;
  char *block = NULL, *duped_line;
  int c, i;
  FILE *fp;
  item_t **new_items;

  if (!filename || !item_count || !items)
    return 0;

  if (filename[0] == '-' && filename[1] == 0)
    {
      fp = stdin;
    }
  else
    {
      fp = fopen (filename, "r");
      if (!fp)
        {
          log_perror (filename);
          return 0;
        }
    }

  while ((c = config_next (fp, config_file_options, &line)))
    {
      switch (c)
        {
          case 'd': // Display
            if (!display_set (line))
              log_error ("%s: Failed to set display to '%s'\n",
                         filename, line);
            break;

          case 'i': // Item
            duped_line = strdup (line);
            if (!duped_line)
              {
                log_perror ("configure_read_file: strdup");
                break;
              }

            if (config_read_block (fp, &block) < 0)
              {
                free (duped_line);
                break;
              }

            new_items = items_add (item_count, *items, duped_line, block);
            if (new_items)
              *items = new_items;
            else
              {
                log_error ("%s: Failed to add item '%s'\n",
                           filename, duped_line);
                if (block)
                  free (block);
              }

            free (duped_line);
            block = NULL;
            break;

          case 'l': // Log
            i = log_stream_spec (line, &line) & ~configure_set_log_files;

            while (isspace (*line))
              line++;

            if (*line)
              log_open_file (line, i);
            else
              log_error ("%s: log: expected filename\n", filename);
            break;

          case 's': // Suppress
            log_suppress (log_stream_spec (line, NULL));
            break;

          case 'k': // Kill delay
            if (! (configure_set_options & CONFIGURE_KILL_DELAY))
              configure_kill_delay = atol (line);
        }
    }

  if (block)
    free (block);

  if (fp != stdin)
    fclose (fp);

  return 1;
}



int
configure_get_kill_delay (void)
{
  return configure_kill_delay;
}
