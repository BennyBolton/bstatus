const Output = require("../output");
const JSONStream = require("JSONStream");


function padText(text, pad) {
    while (--pad >= 0) text += " ";
    return text;
}


function sourceColor(source) {
    let color = source.text[0] && source.text[0].color
    if (color) {
        if (color.length == 6) return "#" + color;
        return `#${color[0]}${color[0]}${color[1]}${color[1]}${color[2]}${color[2]}`;
    }
}


function getText(segment) {
    let text = segment.text.replace(/[&<>"']/g, str => {
        let code = str.charCodeAt(0);
        return `&#${code};`;
    });
    if (segment.color) {
        return `<span color="#${segment.color}">${text}</span>`
    } else {
        return segment.text;
    }
}


class I3Bar extends Output {
    constructor(config) {
        super();
        this.config = config || {};
        this.first = true;
        process.stdout.write("{ \"version\": 1, \"click_events\": true }\n[\n");
        process.stdin.pipe(JSONStream.parse("*")).on("data", data => {
            let match = data.name && data.name.match(/^index_(\d+)$/);
            if (match) this.click(+match[1], data.button);
        });
    }

    display(sources) {
        process.stdout.write((this.first ? "" : ",") + JSON.stringify(
            sources.map((source, i) => ({
                name: `index_${i}`,
                separator: true,
                separator_block_width: this.config.separatorWidth,
                align: "center",
                min_width: source.width ? padText("", source.width) : undefined,
                color: sourceColor(source),
                full_text: source.text.map(getText).join(""),
                short_text: source.text.filter(i => !i.optional).map(getText).join(""),
                markup: "pango"
            }))
        ) + "\n");
        this.first = false;
    }
}


module.exports = I3Bar;