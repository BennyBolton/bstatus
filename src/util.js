const child_process = require("child_process");
const XRegExp = require("xregexp");


class Defer {
    constructor(timeout) {
        this.timeout = timeout || 0;
        this.action = null;
        this._update = () => {
            this.action = null;
            this.cb();
        }
    }

    update(cb) {
        this.cb = cb;
        if (this.action === null) {
            this.action = setTimeout(this._update, this.timeout);
        }
    }
}


const _resultCache = new Map();
function resultCache(id, timeout, cb) {
    if (!cb) {
        cb = timeout;
        timeout = 100;
    }

    let entry;
    if (_resultCache.has(id)) {
        entry = _resultCache.get(id);
    } else {
        entry = { lastUpdate: 0, value: null };
        _resultCache.set(id, entry);
    }

    let now = Date.now();
    if (now > entry.lastUpdate + timeout) {
        entry.lastUpdate = now;
        entry.value = cb();
    }
    return entry.value;
}


function alignTimeout(delay, cb) {
    let time = Date.now() % delay;
    return setTimeout(cb, delay - time);
}


function parseColor(color) {
    let denom = 15, width = 1;
    if (color.length > 3) {
        denom = 255; width = 2;
    }
    return [0, 1, 2].map(i =>
        parseInt("0x" + color.substring(i * width, i * width + width)) / denom
    );
}


function makeCondition(match) {
    let value = +match._value;
    let text = match._text.replace(/\\./, str => str[1]);
    switch (match._suffix) {
        case "K": value *= 1e3; break;
        case "M": value *= 1e6; break;
        case "G": value *= 1e9; break;
        case "T": value *= 1e12; break;
        case "P": value *= 1e15; break;
        case "E": value *= 1e18; break;
        case "Z": value *= 1e21; break;
        case "Y": value *= 1e24; break;
    }
    switch (match._op) {
        case "=":  return x => x == value ? text : "";
        case "!=": return x => x != value ? text : "";
        case "<":  return x => x <  value ? text : "";
        case ">":  return x => x >  value ? text : "";
        case "<=": return x => x <= value ? text : "";
        case ">=": return x => x >= value ? text : "";
    }
}
makeCondition.regex = `
    (?<_op> !?=|[<>]=?)
    (?<_value> \\d+(\\.\\d+)?)
    (?<_suffix> [KMGTPEZY])?
    (\\((?<_text> ([^\\\\\\)]+|\\.)*)\\))
`;


function exec(cmd) {
    return new Promise((resolve, reject) => {
        child_process.exec(cmd, (err, res) => {
            if (err) {
                reject(err);
            } else {
                resolve(res.trim());
            }
        });
    });
}


function fork(cmd) {
    child_process.spawn("nohup", ["nohup", "bash", "-c", cmd], { stdio: null });
}


module.exports = {
    Defer,
    resultCache,
    alignTimeout,
    parseColor,
    makeCondition,
    exec,
    fork
};