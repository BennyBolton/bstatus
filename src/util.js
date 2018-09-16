const child_process = require("child_process");
const XRegExp = require("xregexp");


const sizePrefixes = [
    "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"
];
const speedPrefixes = [
    "B/s\0", "kB/s", "MB/s", "GB/s", "TB/s",
    "PB/s", "EB/s", "ZB/s", "YB/s"
];


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


class Formatter {
    constructor(spec, format) {
        this.comps = [];
        let match;
        while (match = XRegExp.exec(format, spec)) {
            this.comps.push({
                prefix: format.substring(0, match.index),
                match
            });
            format = format.substring(match.index + match[0].length);
        }
        this.suffix = format;
    }

    format(cb) {
        return this.comps.reduce(
            (str, { prefix, match }) => str + prefix + (cb(match) || ""),
            ""
        ) + this.suffix;
    }
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
    Formatter,
    formatSize,
    formatSpeed,
    formatPortion,
    ensureWidth,
    resultCache,
    alignTimeout,
    parseColor,
    exec,
    fork
};