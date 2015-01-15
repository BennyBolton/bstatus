#ifndef __BSTATUS_FORMAT_H__
#define __BSTATUS_FORMAT_H__


/*
  Function type to use for expanding format specifyers
  The function should increment *spec as neccessary to point to the end of the
  format specifyer, and return the string generated
*/
typedef const char *(*format_callback) (const char **spec);


/*
  Generate a string containing format specificated (must begin with %) into *buf
  **buf is reallocated as neccessary
  Will always use '%' for substrings %%, and no call to func is made
  Note that if the formatted string is empty, *buf may be NULL
 */
int
format_string (char **buf, const char *format, format_callback func);


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
