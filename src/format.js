const XRegExp = require("xregexp");


const sizePrefixes = [
    "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
];
const speedPrefixes = [
    "B/s\0", "kB/s", "MB/s", "GB/s", "TB/s",
    "PB/s", "EB/s", "ZB/s", "YB/s"
];


function makeFormatter(rules, line) {
    let matches = rules
        .map(rule => ({ rule, match: XRegExp.exec(line, rule.regex) }))
        .filter(x => x.match);
    let segments = [], prefix = "", start = 0;
    while (matches.length > 0) {
        let next = matches.reduce(
            (a, b) => a.match.index < b.match.index ? a : b
        );
        prefix += line.substring(start, next.match.index);
        start = next.match.index + next.match[0].length;
        let format = next.rule.format;
        if (typeof format == "function") format = format(next.match);
        if (typeof format == "function") {
            segments.push([prefix, format]);
            prefix = "";
        } else {
            prefix += format;
        }
        matches = matches.filter(x => {
            if (x.match.index < start) {
                x.match = XRegExp.exec(line, x.rule.regex, start);
            }
            return x.match;
        });
    }

    return {
        segments,
        suffix: prefix + line.substring(start),
        format() {
            return this.segments.reduce(
                (str, [prefix, format]) => str + prefix + format(...arguments),
                ""
            ) + this.suffix;
        }
    };
}


function formatSize(size) {
    let prefix = 0;
    size = Math.round(parseInt(size) / 100);
    while (size >= 10000) {
        size = Math.round(size / 1000);
        ++prefix;
    }
    let value = `${Math.floor(size / 10)}.${size % 10}`;
    return `${ensureWidth(value, 5)} ${sizePrefixes[prefix]}`;
}


function formatSpeed(size) {
    let prefix = 0;
    size = Math.round(parseInt(size) * 10);
    while (size >= 10000) {
        size = Math.round(size / 1000);
        ++prefix;
    }
    let value = `${Math.floor(size / 10)}.${size % 10}`;
    return `${ensureWidth(value, 5)} ${speedPrefixes[prefix]}`;
}


function formatPortion(value, denom, acc) {
    value = value || 0;
    denom = denom ? +denom : 100;
    acc = acc ? +acc : 0;
    let result = (value * denom).toFixed(acc);
    return ensureWidth(result, denom.toFixed(acc).length);
}


function ensureWidth(value, width) {
    while (value.length < width) value += "\0";
    return value;
}


module.exports = {
    makeFormatter,
    formatSize,
    formatSpeed,
    formatPortion,
    ensureWidth
};