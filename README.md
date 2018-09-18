BSTATUS
=======
Version 1.0.0

bstatus is a simple and extensible status line generator.

BUILDING
--------

To build bstatus, run `npm install && npm run build`. This will minify bstatus
into a single node script, and add the exec flag

USAGE
-----

bstatus can be run by running the created node script with `./bstatus.js`, which
by default will try to use the config file at `~/.bstatus/config.js`, but an
alternative config file can be used by running
`./bstatus.js /path/to/config.js`.

CONFIGURING
-----------

The configuration files are regular node scripts written in javascript, and
`require` can be used to include other files e.g. for custom sources.

Example configuration:

config.js:
```javascript
configure({
    deferTimeout: 50,

    output: new Output.I3Bar({
        separatorWidth: 30
    }),

    sources: [
        new (require("./ip"))(),
        new Source.Network("Net: %rs / %ts"),
        new Source.Cpu("Cpu: %.1p%%[ (%9(/)C)]"),
        new Source.Memory("Memory: %mu"),
        new Source.Memory("Swap: %su"),
        new Source.Clock("[%a %d %b ]%I:%M:%S %p ")
    ]
})
```

ip.js:
```javascript
class Ip extends Source.Repeat {
    constructor() {
        super(30000);
    }

    update() {
        return Util.exec("curl https://api.ipify.org")
            .then(ip => `Ip: ${ip}`);
    }

    click(button) {
        switch (button) {
            case 1: Util.fork("xdg-open https://google.com/"); break;
        }
    }
}

module.exports = Ip;
```

All text output is can include formatting for specifying color using the tag
`{xxx}` or `{xxxxxx}` for 3 or 6 digit hex codes respectively, or `{}` to reset
the color to default. Also `[...]` can be used to specify optional components
which will be stripped if the status line is too long to be displayed. Null
characters (`\0`) can be used to specify padding. They are not outputed in
the final status line, but are counted and used as the width of the field.

The following items are attached to the global object to aid in configuring:

* `configure(config: {}): void` -
    Pass configuration for the program. The supported options are:

    * `deferTimeout: number` -
        timeout in milliseconds between a source updating and updating the
        status line, for use in ensuring sources can be updated simultaneously.

    * `output: Output` -
        The output driver to use.

    * `sources: Source[]` -
        An array of sources to add to the status line.

* `class Output` -
    For use creating custom output drivers (see below). The following outputs
    are provided as static members:

    * `Output.Standard: Output` -
        An output driver that writes to standard output. Uses basic colors and
        overwrites the same line when run interactively, else strips colors and
        writes a new line for each updated status line.

    * `Output.I3Bar: Output` -
        An output driver that outputs in json for use with i3bar. Supports
        click events through stdin.

* `class Source` -
    For use creating custom sources (see below). The following sources are
    provided as static members:

    * `Source.Clock: Source` -
        Uses strftime styled formatting

    * `Source.Cpu: Source` -
        Show CPU utilization. Supports the following format specifiers:
        * `%[<denominator>][.<accuracy>]p` -
            Shows the total utilization, out of _denominator_ (default: 100) to
            _accuracy_ decimal points (default: 0). Supports conditional
            statements (see below).
        * `%<core>[<denominator>][.<accuracy>]c` -
            Shows the utilization of the given _core_, out of _denominator_
            (default: 100) to _accuracy_ decimal points (default: 0). Supports
            conditional statements (see below).
        * `%[<denominator>][.<accuracy>][(<separator>)]c` -
            Shows the utilization of each core, out of _denominator_
            (default: 100) to_accuracy_ decimal points (default: 0),
            separated by _separator_ (default ", ").
        * `%%` -
            A literal percentage character.

    * `Source.DiskUsage: Source` -
        Show disk usage. Format specifiers are of the form '%%' for a literal
        percentage character, or `%[(<path>)]<value>`, where _path_ is a mount
        point to use (default: root), and _value_ is one of:
        * `t` - Total disk space.
        * `u` - Used disk space.
        * `a` - Available disk space.
        * `p` - Used disk space, as a percentage.
        * `P` - Available disk space, as a percentage.
        The `t`, `u`, and `a` specifiers will be human readable, but `T`, `U`,
        and `A` can be used respectively to display the value as a raw figure
        in bytes. Supports conditional statements (see below).

    * `Source.Memory: Source` -
        Show memory utilization. Format specifiers are of the form
        `%%` for a literal percentage character, or `%<source><value>` where
        _source_ is one of 'm', 's', or 't' forphysical memory, swap space,
        and total memory respectively, and _value_ is one of:
        * `u` - Used memory
        * `a` - Available memory, excluding cached memory
        * `f` - Free (Unallocated) memory
        * `t` - Total memory
        These specifiers will be human readable, but `U`, `A`, `F`, and `T` can
        be used to display the appropriate value as a raw figure in bytes.
        The format specifier `%[denominator][.accuracy]p<source><value>` can br
        used to display a value out of _denominator_ (default: 100) to
        _accuracy_ decimal places (default: 0).  Supports conditional
        statements (see below).

    * `Source.Network: Source` -
        Show network utilization. Supports the following format specifiers:
        * `%rb` - Bytes received, human readable
        * `%rB` - Bytes received, raw
        * `%rs` - Rate received, human readable
        * `%rS` - Rate received, raw
        * `%rp` - Packets received
        * `%tb` - Bytes transmitted, human readable
        * `%tB` - Bytes transmitted, raw
        * `%tb` - rate transmitted, human readable
        * `%tB` - rate transmitted, raw
        * `%tp` - Packets transmitted
        * `%%` - A literal percentage character.
        For the appropriate specifiers, the value displayed is the sum of all
        available interfaces. An interface filter can be specified to filter by
        interfaces whose names start with a given value, by using a format
        specifier of the form `%(<filter>)<specifier>`, e.g. `%(eno)rs`.
        Supports conditional statements (see below).

    * `Source.Repeat: Source` -
        Constructed with two arguments, a delay in milliseconds, and a callback
        to be run. The callback should return either nothing, the text to set,
        or a promise for either nothing or the text to set. This callback is
        run with `this` set to the instance of the source, so it's methods can
        be used, as below under `CUSTOM SOURCES`.

    For the above sources that support condition statements, a format
    specification can be made conditional using
    `%<operator><value>(<text>)<specifier>`, where _operator_ is one of `=`,
    `!=`, `<`, `>`, `<=`, or `>=`, and _value_ is an ammount, possibly suffixed
    with either `K`, `M` `G` `T`, `P`, `E`, `Z`, or `Y` for the powers of a
    thousand respectively. _specifier_ is a usual specifier, and if the given
    value compares to the computed value, _text_ will be included in the
    output. For percentage values (such as `%p` in Source.CPU), _denominator_
    and _accuracy_ are unavailable, and a value of 100 is used instead.

* `Format.formatSize(size: number): string` -
    Format a size (in bytes) using an appropriate prefix. Adds null characters
    for padding.

* `Format.formatSpeed(speed: number): string` -
    Format a speed (in bytes per second) using an appropriate prefix. Adds
    null characters for padding.

* `Format.formatPortion(portion: number, denom: number = 100, acc: number = 0): string` -
    Format a portion (from 0 to 1), such as percentage values. Adds null
    characters for padding.

* `Format.ensureWidth(str: string, width: number): string` -
    Add null characters to _str_ until it is at least _width_ long.

* `Util.exec(cmd: string): Promise<string>` -
    Helper function for running a command. Returns a promise for the trimmed
    stdout.

* `Util.fork(cmd: string): void` -
    Helper function like exec, but for use when the result isn't required, e.g.
    for launching a web page. the command will be run through nohup


CUSTOM SOURCES
--------------

Custom sources can be created by extending the `Source` class. The following
methods are provided to update the sources status:
* `setText(text: string): void` -
    Set the text to display, including formatted tokens.
* `setError(error: Error): void` -
    Display the error in place of the sources status

Click events can be captured by overriding the `click(button: number): void`
method. The button is mapped to, starting from 1, left mouse button, middle
mouse button, right mouse button, mouse wheel up, mouse wheel down.

The `Source.Repeat` source can be extended to make regularly repeating sources,
in which the `update(): void | string | Promise<void | string>` method can be
overridden, in place of the callback given to the constructor.

CUSTOM OUTPUTS
--------------

Custom output drivers can be created by extending the `Output` class. the
`display(sources: ISource[]): void` method can be overridden to update the
status line, with ISource being defined as:

```typescript
interface ISource {
    width: number; // This minimum width in characters to display for the field
    text: {
        optional: boolean; // Whether this part of the text can be culled if
                           // the status line is too long
        color: string; // The color to use for this segment of text
        text: string; // The text itself
    }[]
}
```

The `click(source: number, button: number): void` method can be used to signal
when a click event occurs, with _source_ being the index in the _sources_
array passed to the _display_ method.