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

#ifndef __BSTATUS_FORMAT_H__
#define __BSTATUS_FORMAT_H__


/*
  Function type to use for expanding format specifyers
  The function should increment *spec as neccessary to point to the end of the
  format specifyer, and return the string generated
*/
typedef const char *(*format_callback) (const char **spec, void *data);


/*
  Generate a string containing format specificated (must begin with %) into *buf
  **buf is reallocated as neccessary
  data is passed to func
  Will always use '%' for substrings %%, and no call to func is made
  Note that if the formatted string is empty, *buf may be NULL
 */
int
format_string (char **buf, const char *format,
               format_callback func, void *data);


/*
  Attempts to match a comparison of the format {<,>,<=,>=,=,!=}N<str> where N
  is an integer and str is some string
  If the comparison is matched, *line is incremented to the end of the substring
  and if value compared appropriately to N is true, the string str is returned,
  else an empty string.
  NULL is returned if a comparison is not matched
*/
const char*
format_read_comparison (const char **line, long int value);


#endif // __BSTATUS_FORMAT_H__
