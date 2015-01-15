#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"


static char *current_line = NULL;
static size_t current_n = 0;
static int current_peek = 0;



/*
  Removes trailing white space on current line
*/
static void
config_strip_line (void)
{
  char *line = current_line;
  char *last_white = NULL;
  int seen_white = 0;

  if (!line)
    return;

  while (*line)
    {
      if (!seen_white && isspace (*line))
        {
          seen_white = 1;
          last_white = line;
        }
      else if (seen_white && !isspace (*line))
        seen_white = 0;

      line++;
    }

  if (last_white)
    *last_white = 0;
}



int
config_next (FILE *fp, const config_option *options, const char **line)
{
  const char *option;
  size_t len;
  int i;

  if (!fp || !options || !line)
    return 0;

  while ((*line = config_skip_line (fp)))
    {
      config_strip_line ();

      while (isspace (**line))
        (*line)++;

      if (**line == 0 || **line == '#')
        continue;

      option = *line;
      len = 0;

      while (**line && !isspace (**line))
        {
          (*line)++;
          len++;
        }

      while (isspace (**line))
        (*line)++;

      for (i = 0; options[i].name; i++)
        if (strncmp (option, options[i].name, len) == 0
            && options[i].name[len] == 0)
          return options[i].value;

      log_error ("config_next: '%s' is not a valid option\n", option);
    }

  return 0;
}



const char*
config_peek_line (FILE *fp)
{
  if (!fp)
    return NULL;

  if (current_peek)
    return current_line;

  if (getline (&current_line, &current_n, fp) < 0)
    {
      if (current_line)
        {
          free (current_line);
          current_line = NULL;
        }

      current_n = 0;
      current_peek = 0;
      return NULL;
    }

  current_peek = 1;
  return current_line;
}



const char*
config_skip_line (FILE *fp)
{
  const char *line;

  line = config_peek_line (fp);
  if (line)
    current_peek = 0;

  return line;
}



int
config_read_block (FILE *fp, char **block)
{
  char *rblock;
  const char *line;
  size_t len = 0, add, lines = 0;
        
  if (!block)
    return 0;

  if (*block)
    **block = 0;

  while ((line = config_peek_line (fp)) && isspace (line[0]))
    {
      add = strlen (line);

      rblock = realloc (*block, len + add + 1);
      if (!rblock)
        {
          log_perror ("config_read_block: realloc");
          return -1;
        }

      *block = rblock;
      strncpy (*block + len, line, add);
      len += add;
      (*block)[len] = 0;

      lines++;
      config_skip_line (fp);
    }

  return lines;
}



int
config_line_next (const char **line, const config_option *options,
                  const char **value, int *len)
{
  const char *option;
  int i, olen;

  if (!line || !options || !value || !len)
    return 0;

  while (**line)
    {
      // Determine the option (as string)

      while (isspace (**line))
        (*line)++;

      option = *line;
      olen = 0;

      while (**line && !isspace (**line) && **line != '=')
        {
          olen++;
          (*line)++;
        }

      // Determine the value associated

      if (**line == 0 || isspace (**line))
        {
          *value = *line;
          *len = 0;
        }
      else
        {
          (*line)++;

          *value = *line;
          *len = 0;

          if (**line == '\"')
            {
              (*line)++;
              (*value)++;
              while (**line && **line != '\"')
                {
                  (*line)++;
                  (*len)++;
                }
              if (**line)
                (*line)++;
            }
          else
            {
              while (**line && !isspace (**line))
                {
                  (*line)++;
                  (*len)++;
                }
            }
        }

      // Lookup the option code

      for (i = 0; options[i].name; i++)
        if (strncmp (option, options[i].name, olen) == 0
            && options[i].name[olen] == 0)
          return options[i].value;

      log_error ("config_line_next: '%.*s' is not a valid option\n",
                 olen, option);
    }

  return 0;
}
