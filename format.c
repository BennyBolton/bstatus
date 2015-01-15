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

#include "format.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"



static const char*
_format_token (const char **position, size_t *token_len, format_callback func)
{
  int escaped = 0;
  const char *str;

  if (!position || !*position || !**position || !token_len || !func)
    return NULL;

  if (**position == '%')
    {
      (*position)++;
      if (**position == '%')
        escaped = 1;
      else if (**position == 0)
        return NULL;
      else
        {
          str = func (position);
          if (str)
            {
              *token_len = strlen (str);
              return (str);
            }
        }
    }

  *token_len = 0;
  str = *position;

  while (**position && (escaped || **position != '%'))
    {
      escaped = 0;
      (*token_len)++;
      (*position)++;
    }

  return str;
}



int
format_string (char **buf, const char *format, format_callback func)
{
  const char *token;
  char *new_buf;
  size_t token_len;
  size_t buf_len = 0;

  if (!buf || !format || !func)
    {
      log_error ("format_string: Invalid argument\n");
      return 0;
    }

  if (*buf)
    **buf = 0;

  while ((token = _format_token (&format, &token_len, func)))
    if (token_len > 0)
      {
        new_buf = realloc (*buf, buf_len + token_len + 1);
        if (!new_buf)
          {
            log_perror ("format_string: realloc");
            if (*buf)
              **buf = 0;
            return 0;
          }

        *buf = new_buf;
        strncpy (*buf + buf_len, token, token_len);
        buf_len += token_len;
        (*buf)[buf_len] = 0;
      }

  return 1;
}



/*
  Values for defining comparisons
*/
enum format_comparison_t
  {
    FORMAT_COMPARE_LT, // <
    FORMAT_COMPARE_LE, // <=

    FORMAT_COMPARE_GT, // >
    FORMAT_COMPARE_GE, // >=

    FORMAT_COMPARE_EQ, // =
    FORMAT_COMPARE_NE, // !=
  };


const char*
format_read_comparison (const char **line, long int value)
{
  static char buf[128];

  int comparison = 0;
  int n, len = 0;
  const char *start;

  if (!line || !*line)
    return NULL;

  // Read the comparison

  switch (**line)
    {
      case '<':
        (*line)++;
        if (**line == '=')
          {
            (*line)++;
            comparison = FORMAT_COMPARE_LE;
          }
        else
          comparison = FORMAT_COMPARE_LT;
        break;

      case '>':
        (*line)++;
        if (**line == '=')
          {
            (*line)++;
            comparison = FORMAT_COMPARE_GE;
          }
        else
          comparison = FORMAT_COMPARE_GT;
        break;

      case '=':
        (*line)++;
        if (**line == '=')
          (*line)++;

        comparison = FORMAT_COMPARE_EQ;
        break;

      case '!':
        (*line)++;
        if (**line == '=')
          (*line)++;

        comparison = FORMAT_COMPARE_NE;
        break;

      default:
        return NULL;
    }

  // Read the value

  n = strtol (*line, (char**) line, 0);

  // Read the string

  if (**line == '<')
    {
      (*line)++;
      start = *line;

      while (**line && **line != '>')
        {
          len++;
          (*line)++;
        }

      if (**line)
        (*line)++;
    }

  // Make the comparison

  switch (comparison)
    {
      case FORMAT_COMPARE_LT:  n = (value <  n); break;
      case FORMAT_COMPARE_LE:  n = (value <= n); break;
      case FORMAT_COMPARE_GT:  n = (value >  n); break;
      case FORMAT_COMPARE_GE:  n = (value >= n); break;
      case FORMAT_COMPARE_EQ:  n = (value == n); break;
      case FORMAT_COMPARE_NE:  n = (value != n); break;
    }

  if (n)
    {
      if (len < sizeof (buf))
        {
          strncpy (buf, start, len);
          buf[len] = 0;
        }
      else
        {
          strncpy (buf, start, sizeof (buf));
          buf[sizeof (buf) - 1] = 0;
        }
    }
  else
    *buf = 0;

  return buf;
}
