const textParseRegexp = /[\[\]]|\{\}|\{([a-fA-F0-9]{3}){1,2}\}|\\./;


class Source {
    constructor() {
        this._width = 0;
        this._text = [];
    }

    setWidth(width) {
        if (typeof width == "string") {
            let text = width;
            width = 0;
            let token;
            while (token = text.match(textParseRegexp)) {
                width += token.index + (token[0][0] == "\\" ? 1 : 0);
                text = text.substring(token.index + token[0].length);
            }
            width += text.length;
        }
        this._width = width;
        this._update();
    }

    setText(text) {
        text = text ? text.toString() : "";

        this._text.length = 0;

        let block = {
            optional: false,
            color: null,
            text: ""
        };
        let newBlock = {
            optional: false,
            color: null
        }
        let token;
        while (token = text.match(textParseRegexp)) {
            block.text += text.substring(0, token.index);
            let changed = false;
            switch (token[0][0]) {
                case "\\":
                    block.text += token[0][1];
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
                    newBlock.color = token[0].substring(1, token[0].length - 1) || null;
                    changed = block.color != newBlock.color;
                    break;

                default:
                    throw new Error("Source.setText: Internal error, bad token: " + token[0])
            }
            if (changed) {
                if (block.text.length > 0) {
                    this._text.push({
                        optional: block.optional,
                        color: block.color,
                        text: block.text,
                    });
                }
                block.optional = newBlock.optional;
                block.color = newBlock.color;
                block.text = "";
            }
            text = text.substring(token.index + token[0].length);
        }
        block.text += text;
        if (block.text.length > 0) {
            this._text.push(block);
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