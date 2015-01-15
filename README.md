BSTATUS
=======
Version 0.1.0

bstatus is a simple and extensible status line generator.

BUILDING
--------

To build bstatus, run `make`. This will compile all c sources, produce the
bstatus binary, as well as the binary for bstatus-sleep, and compresses the man
pages.

INSTALLING
----------

To install bstatus, run `make install`. This will install the binaries to
`/usr/bin`, the man pages to their appropriate locations in `/usr/share/man`,
the default configuration to `/etc`, and the python module for bstatus_display
to its appropriate location in `/usr/lib/python3.4`.

USAGE
-----

bstatus can be run simply by calling `bstatus`. See the man page for
bstatus.conf for help on configuring bstatus.
