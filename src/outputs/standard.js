const Output = require("../output");
const { parseColor } = require("../util");


function padText(text, pad, prepad = 0) {
    pad -= prepad;
    while (--prepad >= 0) text = ` ${text}`;
    while (--pad >= 0) text += " ";
    return text;
}


class Standard extends Output {
    constructor() {
        super();
        this.lastWidth = 0;
    }

    display(sources) {
        let showOptional = true;
        if (process.stdout.isTTY) {
            let width = sources.reduce(
                (count, source) => {
                    return count + 3 + Math.max(source.width, source.text.reduce(
                        (length, text) => length + text.text.length, 0
                    ));
                },
                -3
            );
            showOptional = width < process.stdout.columns;
        }
        let statusLine = "", totalWidth = 0, first = true;
        for (let source of sources) {
            let sourceText = "", width = 0;
            for (let { optional, color, text } of source.text) {
                if (optional && !showOptional) continue;
                if (color && process.stdout.isTTY) {
                    color = parseColor(color)
                        .map((comp, i) => comp > 0.5 ? 1 << i : 0)
                        .reduce((a, b) => a + b, 30);
                    sourceText += `\x1b[${color}m${text}\x1b[m`;
                } else {
                    sourceText += text;
                }
                width += text.length;
            }
            if (showOptional && width < source.width) {
                let pad = source.width - width;
                sourceText = padText(sourceText, pad, Math.floor(pad / 2));
                totalWidth += pad;
            }
            if (!first) {
                statusLine += " | ";
                totalWidth += 3;
            }
            first = false;
            statusLine += (sourceText || "(null)");
            totalWidth += width;
        }
        if (process.stdout.isTTY) {
            let pad = Math.max(0, this.lastWidth - totalWidth);
            statusLine = `\r${padText(statusLine, pad)}`;
        } else {
            statusLine += "\n";
        }
        this.lastWidth = totalWidth;

        process.stdout.write(statusLine);
    }
}


module.exports = Standard;