const Repeat = require("./repeat");
const fs = require("fs");
const XRegExp = require("xregexp");
const { resultCache, makeCondition } = require("../util");
const { makeFormatter, formatSize, formatPortion } = require("../format");


const parseMeminfoRegexp = XRegExp(`
    ^
    \s* (?<key> [^:]+):
    \s* (?<value> .*[^\s])
    \s*$
`, "x");


const formatRules = [
    {
        regex: /%%/,
        format: "%"
    },
    {
        regex: XRegExp(`%
            (?<spec> [mst][uaftUAFT])
        `, "x"),
        format(match) {
            let value;
            switch (match.spec) {
                case "mU": value = getValue("mu");
                case "mA": value = getValue("ma");
                case "mF": value = getValue("mf");
                case "mT": value = getValue("mt");
                case "sU": value = getValue("su");
                case "sA": value = getValue("sa");
                case "sF": value = getValue("sf");
                case "sT": value = getValue("st");
                case "tU": value = getValue("tu");
                case "tA": value = getValue("ta");
                case "tF": value = getValue("tf");
                case "tT": value = getValue("tt");
            }

            if (value) {
                return usage => value(usage).toString();
            } else {
                value = getValue(match.spec);
                return usage => formatSize(value(usage));
            }
        }
    },
    {
        regex: XRegExp(`%
            (?<denom> \\d+)?
            (\\.(?<acc> \\d+))?
            p
            (?<spec> [mst][uaft])
        `, "x"),
        format(match) {
            let denom = +match.denom || 100;
            let acc = +match.acc || 0;
            let value = getValue(match.spec);
            let total = getValue(match.spec[0] + "t");
            return usage => formatPortion(
                value(usage) / (total(usage) || 1), denom, acc
            );
        }
    },
    {
        regex: XRegExp(`%
            ${makeCondition.regex}
            (?<percent> p)?
            (?<spec> [mst][uaft])
        `, "x"),
        format(match) {
            let condition = makeCondition(match);
            let value = getValue(match.spec);
            if (match.percent) {
                let amount = value;
                let total = getValue(match.spec[0] + "t");
                value = usage => 100 * amount(usage) / (total(usage) || 1);
            }
            return usage => condition(value(usage));
        }
    }
];


function getMemoryStats() {
    return resultCache("Source.Memory", () =>
        new Promise((resolve, reject) => {
            fs.readFile("/proc/meminfo", (err, data) => {
                if (err) {
                    reject(err);
                } else {
                    let res = {};
                    try {
                        for (let line of data.toString().split("\n")) {
                            let match = XRegExp.exec(line, parseMeminfoRegexp);
                            if (match) {
                                res[match.key] = parseInt(match.value) * 1000;
                            }
                        }
                    } catch (err) {
                        return reject(err);
                    }
                    resolve(res);
                }
            });
        })
    );
}


function getValue(value) {
    if (value[0] == "t") {
        let v1 = getValue("m" + value[1]);
        let v2 = getValue("s" + value[1]);
        return usage => v1(usage) + v2(usage);
    }

    switch (value) {
        case "mu": return usage => usage.MemTotal - usage.MemFree;
        case "ma": return usage => usage.MemAvailable;
        case "mf": return usage => usage.MemFree;
        case "mt": return usage => usage.MemTotal;

        case "su": return usage => usage.SwapTotal - usage.SwapFree;
        case "sa": return usage => usage.SwapFree + usage.SwapCached;
        case "sf": return usage => usage.SwapFree;
        case "st": return usage => usage.SwapTotal;
    }
}


class Memory extends Repeat {
    constructor(format) {
        super(1000);
        this.formatter = makeFormatter(formatRules, format);
    }

    update() {
        return getMemoryStats().then(usage => {
            this.setText(this.formatter.format(usage));
        });
    }
}


module.exports = Memory;