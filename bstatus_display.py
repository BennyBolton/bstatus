#!/usr/bin/env python3

##
## This file is part of bstatus.
##
## bstatus is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## bstatus is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with bstatus.  If not, see <http://www.gnu.org/licenses/>.
##

""" Part of bstatus, used for custom display drivers """

from os import fdopen


# Special value for use of the default color
COLOR_DEFAULT = 0x1000000


# Stream to read status lines from
_in_stream = fdopen (3)

# Stream to write events to
_out_stream = fdopen (4, "w", 1)


# States for parse_item
_STATE_READING_TOKEN       = 0
_STATE_ESCAPED             = 1
_STATE_READING_COLOR       = 2
_STATE_READING_COLOR_VALUE = 3



def line_ready ():
    """ Sleeps until a status line is ready """

    return True



def read_items ():
    """ Iterates over the items in the current status line.
        each element is an item as a tuple (length, item), where item is the
        item's text, and length is the requested minimum length to display the
        item as """

    while True:
        line = _in_stream.readline ().strip ()

        pos = line.find (':')
        if pos < 0:
            break

        length = 0
        if pos > 0:
            length = int (line[:pos])

        yield (length, line[pos + 1:])



def parse_item (item):
    """ Iterates over the tokens in an item.
        Each element is a token as a tuple (color, option, text), where color is
        a hexadecimal integer of the format 0xRRGGBB, or is COLOR_DEFAULT,
        option is a boolean, specifying whether or not this token is optional,
        and text is the text associated with this token """

    option = False
    color  = COLOR_DEFAULT # Special value for default color
    token  = ""
    state  = _STATE_READING_TOKEN

    for c in item:
        if state == _STATE_READING_TOKEN:
            if c == '[':
                if len (token) > 0:
                    yield (color, option, token)
                    token = ""

                option = True

            elif c == ']':
                if len (token) > 0:
                    yield (color, option, token)
                    token = ""

                option = False

            elif c == '\\':
                state = _STATE_ESCAPED

            elif c == '{':
                if len (token) > 0:
                    yield (color, option, token)
                    token = ""

                state = _STATE_READING_COLOR

            else:
                token += c

        elif state == _STATE_ESCAPED:
            token += c
            state = _STATE_READING_TOKEN

        elif state == _STATE_READING_COLOR:
            if c == '}':
                color = COLOR_DEFAULT
                state = _STATE_READING_TOKEN
            else:
                color = 0
                state = _STATE_READING_COLOR_VALUE

        if state == _STATE_READING_COLOR_VALUE:
            if c == '}':
                state = _STATE_READING_TOKEN
            elif c >= '0' and c <= '9':
                color = color * 16 + ord (c) - ord ('0')
            else:
                color = color * 16 + ord (c.upper ()) - ord ('A') + 10

    if len (token) > 0:
        yield (color, option, token)



def do_click (index, button, position = -1):
    """ Pass a click event to bstatus for the item with index given, for the
        mouse button given at index position within the status line. """

    _out_stream.write ("click {} {} {}\n".format (index, button, position))
