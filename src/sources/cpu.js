const Repeat = require("./repeat");
const fs = require("fs");
const XRegExp = require("xregexp");
const { exec, resultCache, makeCondition } = require("../util");
const { makeFormatter, formatPortion } = require("../format");


const parseCpuStatsRegexp = XRegExp(`^
    \\s* cpu(?<index> \\d*)
    \\s+ (?<user> \\d+)
    \\s+ \\d+
    \\s+ (?<system> \\d+)
    \\s
`, "x");


const formatRules = [
    {
        regex: /%%/,
        format: "%"
    },
    {
        regex: XRegExp(`%
            (?<denom> \\d+)?
            (\\.(?<acc> \\d+))?
            p
        `, "x"),
        format(match) {
            let denom = +match.denom || 100;
            let acc = +match.acc || 0;
            return usage => formatPortion(usage[0], denom, acc);
        }
    },
    {
        regex: XRegExp(`%
            (?<core> \\d+)
            (-(?<denom> \\d+))?
            (\\.(?<acc> \\d+))?
            c
        `, "x"),
        format(match) {
            let index = +match.core;
            let denom = +match.denom || 100;
            let acc = +match.acc || 0;
            return usage => formatPortion(usage[index] || 0, denom, acc);
        }
    },
    {
        regex: XRegExp(`%
            (?<denom> \\d+)?
            (\\.(?<acc> \\d+))?
            (\\((?<sep> ([^\\\\\\)]+|\\.)*)\\))?
            C
        `, "x"),
        format(match) {
            let denom = +match.denom || 100;
            let acc = +match.acc || 0;
            let sep = match.sep && match.sep.replace(/\\./, str => str[1]);
            if (sep == null) sep = ", ";
            return usage => {
                let res = "";
                for (let i = 1; i <= usage.count; ++i) {
                    if (i > 1) res += sep;
                    res += formatPortion(usage[i], denom, acc);
                }
                return res;
            }
        }
    },
    {
        regex: XRegExp(`%
            ${makeCondition.regex}
            p
        `, "x"),
        format(match) {
            let condition = makeCondition(match);
            return usage => condition(usage[0] * 100);
        }
    },
    {
        regex: XRegExp(`%
            ${makeCondition.regex}
            (?<core> \\d+)
            c
        `, "x"),
        format(match) {
            let index = +match.core;
            let condition = makeCondition(match);
            return usage => condition((usage[index] || 0) * 100);
        }
    }
];


function getCpuStats() {
    return resultCache("Source.Cpu", () =>
        new Promise((resolve, reject) => {
            fs.readFile("/proc/stat", (err, data) => {
                if (err) {
                    reject(err);
                } else {
                    let res = { time: Date.now(), count: 0 };
                    try {
                        for (let line of data.toString().split("\n")) {
                            let match = XRegExp.exec(
                                line, parseCpuStatsRegexp
                            );
                            if (match) {
                                let index = 0;
                                if (match.index.length > 0) {
                                    index = +match.index + 1;
                                    if (index > res.count) res.count = index;
                                }
                                res[index] = +match.user + +match.system;
                            }
                        }
                        res[0] /= res.count;
                    } catch (err) {
                        return reject(err);
                    }
                    resolve(res);
                }
            });
        })
    );
}


function clampUsage(usage, time) {
    usage /= time;
    if (usage < 0) usage = 0;
    if (usage > 1) usage = 1;
    return usage;
}


function makeUsage(cur, old, clkTck) {
    let res = { count: cur.count };
    let timeDiff = (old && (cur.time - old.time) || 1) / 1000;
    for (let i = 0; i <= cur.count; ++i) {
        res[i] = old ? clampUsage((cur[i] - old[i]) / clkTck, timeDiff) : 0;
    }
    return res;
}


class Cpu extends Repeat {
    constructor(format) {
        super(500);
        this.formatter = makeFormatter(formatRules, format);
        this.clcTck = exec("getconf CLK_TCK").then(
            val => parseInt(val),
            err => 100
        );
        this.lastStats = null;
    }

    update() {
        Promise.all([this.clcTck, getCpuStats()]).then(([clkTck, stats]) => {
            let usage = makeUsage(stats, this.lastStats, clkTck);
            this.lastStats = stats;
            this.setText(this.formatter.format(usage));
        });
    }
}


module.exports = Cpu;