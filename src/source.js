const XRegExp = require("xregexp");


const textParseRegexp = XRegExp(`
    [\\[\\]] |
    \\{\\} |
    \\{ (?<color> ([a-fA-F0-9]{3}){1,2}) \\} |
    \0 |
    \\\\.
`, "x");


class Source {
    constructor() {
        this._width = 0;
        this._text = [];
    }

    setText(text) {
        text = text ? text.toString() : "";

        this._text.length = 0;
        this._width = 0;

        let block = {
            optional: false,
            color: null,
            text: ""
        };
        let newBlock = {
            optional: false,
            color: null
        }
        let match;
        while (match = XRegExp.exec(text, textParseRegexp)) {
            block.text += text.substring(0, match.index);
            let changed = false;
            switch (match[0][0]) {
                case "\\":
                    block.text += match[0][1];
                    break;

                case "[":
                    changed = !block.optional;
                    newBlock.optional = true;
                    break;

                case "]":
                    changed = block.optional;
                    newBlock.optional = false;
                    break;

                case "{":
                    newBlock.color = match.color || null;
                    changed = block.color != newBlock.color;
                    break;

                case "\0":
                    ++this._width;
                    break;

                default:
                    throw new Error("Source.setText: Internal error, bad token: " + match[0])
            }
            if (changed) {
                if (block.text.length > 0) {
                    this._text.push({
                        optional: block.optional,
                        color: block.color,
                        text: block.text,
                    });
                    this._width += block.text.length;
                }
                block.optional = newBlock.optional;
                block.color = newBlock.color;
                block.text = "";
            }
            text = text.substring(match.index + match[0].length);
        }
        block.text += text;
        if (block.text.length > 0) {
            this._text.push(block);
            this._width += block.text.length;
        }
        this._update();
    }

    setError(error) {
        this._text = [{ optional: false, color: "f00", text: error.toString() }];
    }

    click(button) {}

    attach(programHandle) {
        this._programHandle = programHandle
    }

    _update() {
        this._programHandle && this._programHandle();
    }
}

module.exports = Source;