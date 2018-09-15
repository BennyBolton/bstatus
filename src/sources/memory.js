const Repeat = require("./repeat");
const fs = require("fs");
const { alignTimeout } = require("../util");


/*
    TODO:

    - Percentage values
    - Set width
    - Conditionals, of the form %([<>]|[<>]?=)<ammount>/d+(<text>)<specifier>
        e.g. %<100000({f00})mA
*/


const parseMeminfoRegexp = /^\s*([^:]+):\s*(.*[^\s])\s*$/;
const formatRegexp = /%(%|[mst][uaftUAFT])/g;
const prefixes = ["kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"];


function getMemoryStats() {
    return new Promise((resolve, reject) => {
        fs.readFile("/proc/meminfo", (err, data) => {
            if (err) {
                reject(err);
            } else {
                let res = {};
                try {
                    for (let line of data.toString().split("\n")) {
                        let match = line.match(parseMeminfoRegexp);
                        if (match) {
                            res[match[1]] = parseInt(match[2]);
                        }
                    }
                } catch (err) {
                    return reject(err);
                }
                resolve(res);
            }
        });
    });
}


function getValue(value, stats) {
    switch (value) {
        case "%mU": return stats.MemTotal - stats.MemFree;
        case "%mA": return stats.MemAvailable;
        case "%mF": return stats.MemFree;
        case "%mT": return stats.MemTotal;

        case "%sU": return stats.SwapTotal - stats.SwapFree;
        case "%sA": return stats.SwapFree + stats.SwapCached;
        case "%sF": return stats.SwapFree;
        case "%sT": return stats.SwapTotal;

        case "%tU": return getValue("%mU", stats) + getValue("sU", stats);
        case "%tA": return getValue("%mA", stats) + getValue("sA", stats);
        case "%tF": return getValue("%mF", stats) + getValue("sF", stats);
        case "%tT": return getValue("%mT", stats) + getValue("sT", stats);
    }
}


function formatSize(size) {
    let prefix = 0;
    size *= 10;
    while (size >= 10000) {
        size = Math.round(size / 1000);
        ++prefix;
    }
    return `${Math.floor(size / 10)}.${size % 10} ${prefixes[prefix]}`;
}


class Memory extends Repeat {
    constructor(format) {
        super(1000);
        this.format = format;
    }

    update() {
        return getMemoryStats().then(stats => {
            this.setText(this.format.replace(formatRegexp, str => {
                switch (str) {
                    case "%%": return "%";

                    case "%mU": return getValue(str, stats).toString();
                    case "%mA": return getValue(str, stats).toString();
                    case "%mF": return getValue(str, stats).toString();
                    case "%mT": return getValue(str, stats).toString();
                    case "%sU": return getValue(str, stats).toString();
                    case "%sA": return getValue(str, stats).toString();
                    case "%sF": return getValue(str, stats).toString();
                    case "%sT": return getValue(str, stats).toString();
                    case "%tU": return getValue(str, stats).toString();
                    case "%tA": return getValue(str, stats).toString();
                    case "%tF": return getValue(str, stats).toString();
                    case "%tT": return getValue(str, stats).toString();

                    case "%mu": return formatSize(getValue("%mU", stats));
                    case "%ma": return formatSize(getValue("%mA", stats));
                    case "%mf": return formatSize(getValue("%mF", stats));
                    case "%mt": return formatSize(getValue("%mT", stats));
                    case "%su": return formatSize(getValue("%sU", stats));
                    case "%sa": return formatSize(getValue("%sA", stats));
                    case "%sf": return formatSize(getValue("%sF", stats));
                    case "%st": return formatSize(getValue("%sT", stats));
                    case "%tu": return formatSize(getValue("%tU", stats));
                    case "%ta": return formatSize(getValue("%tA", stats));
                    case "%tf": return formatSize(getValue("%tF", stats));
                    case "%tt": return formatSize(getValue("%tT", stats));
                }
                throw new Error("Memory.update: Internal error: Bad format: " + str);
            }));
        });
    }
}


module.exports = Memory;