#ifndef __BSTATUS_CONFIG_H__
#define __BSTATUS_CONFIG_H__


#include <stdio.h>


/*
  Simple structure for holding a list of options in the configuration file
  The final option should be {NULL, 0}
*/
typedef struct _config_option config_option;


struct _config_option {
  const char *name; // Option name
  int value;        // Value to associate with this option
};



/*
  Reads untill the next line that is a configuration option in options
  Sets *line to the value of the option, and returns the value associated with
  the option
  If the option is unrecognised, an error message is displayed, and
  config_next continues on to the next option
  If the end of the file is reached, returns 0
*/
int
config_next (FILE *fp, const config_option *options, const char **line);


/*
  Reads the next line if neccessary and returns it without processing it
  Returns NULL on error or if the end of the file is reached
*/
const char*
config_peek_line (FILE *fp);


/*
  Typically follows a call to config_peek_line. Skips a line that has been
  peeked at, else reads the next line and skips it
  Returns the skipped line
  Returns NULL on error or if the end of the file is reached
*/
const char*
config_skip_line (FILE *fp);


/*
  Reads the following block from the stream, and stores it in the pointer
  block, calling realloc as neccessary
  A block is defined as none or more lines beginning with a whitespace
  character
  Returns -1 on an allocation error else the ammount of lines read in the
  block
  If *block is not null and no error occurs, *block will always be null
  terminated
  Note that the string may be altered on error
*/
int
config_read_block (FILE *fp, char **block);


/*
  Acts similar to config_next, but instead will instead read the options from a
  string in the format option=value, or option="value containing spaces"
  Note that *value once set may not be NULL terminated, hence *len is also set
*/
int
config_line_next (const char **line, const config_option *options,
                  const char **value, int *len);


#endif // __BSTATUS_CONFIG_H__
