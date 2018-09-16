const Repeat = require("./repeat");
const fs = require("fs");
const XRegExp = require("xregexp");
const { exec, Formatter, formatPortion, resultCache } = require("../util");


const parseCpuStatsRegexp = XRegExp(`
    ^
    \\s* cpu(?<index> \\d*)
    \\s+ (?<user> \\d+)
    \\s+ \\d+
    \\s+ (?<system> \\d+)
    \\s
`, "x");

const formatRegexp = XRegExp(`
    % % |
    % (?<pDenom> \\d+)? (\\.(?<pAccuracy> \\d+))? p |
    % (?<cIndex> \\d+) (-(?<cDenom> \\d+))? (\\.(?<cAccuracy> \\d+))? c |
    % (?<CDenom> \\d+)? (\\.(?<CAccuracy> \\d+))?
        (\\((?<CSeparator> ([^\\\\\\)]+|\\.)*)\\))? C
`, "x");


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
                            let match = XRegExp.exec(line, parseCpuStatsRegexp);
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
        this.formatter = new Formatter(formatRegexp, format);
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
            this.setText(this.formatter.format(match => {
                switch (match[0][match[0].length - 1]) {
                    case "%": return "%";
        
                    case "p":
                        return formatPortion(usage[0], match.pDenom, match.pAccuracy);
        
                    case "c":
                        return formatPortion(
                            usage[match.cIndex] || 0,
                            match.cDenom,
                            match.cAccuracy
                        );
        
                    case "C":
                        let txt = "";
                        let separator = match.CSeparator || ", ";
                        for (let i = 0; i < usage.count; ++i) {
                            if (i > 0) txt += separator;
                            txt += formatPortion(usage[i + 1], match.CDenom, match.CAccuracy);
                        }
                        return txt;
                }
            }));
        });
    }
}


module.exports = Cpu;