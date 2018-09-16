const Repeat = require("./repeat");
const fs = require("fs");
const XRegExp = require("xregexp");
const { resultCache, Formatter, formatSpeed } = require("../util");


const parseNeworkStatsRegexp = XRegExp(`
    ^
    \\s* (?<name> [^:]+):
    \\s* (?<rs> \\d+)
    \\s+ (?<rp> \\d+)
    (\\s+\\d+){6}
    \\s+ (?<ts> \\d+)
    \\s+ (?<tp> \\d+)
    \\s
`, "x");

const formatRegexp = XRegExp(`
    % (?<literal> %) |
    % (\\((?<name> ([^\\\)]+|\\.)*)\\))?
        (?<spec> [rt][sSp])
`, "x");


function getNetworkStats() {
    return resultCache("Source.Network", () =>
        new Promise((resolve, reject) => {
            fs.readFile("/proc/net/dev", (err, data) => {
                if (err) {
                    reject(err);
                } else {
                    let res = { time: Date.now(), stats: [] };
                    try {
                        for (let line of data.toString().split("\n")) {
                            let match = XRegExp.exec(line, parseNeworkStatsRegexp);
                            if (match) {
                                res.stats.push({
                                    name: match.name,
                                    rs: +match.rs,
                                    rp: +match.tp,
                                    ts: +match.ts,
                                    tp: +match.tp
                                });
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


function makeUsage(cur, old) {
    let res = [];
    let oldByName = {};
    let timeDiff = (old && (cur.time - old.time) || 1) / 1000;
    if (old) {
        for (let i of old.stats) {
            oldByName[i.name] = i;
        }
    }
    for (let i of cur.stats) {
        if (oldByName.hasOwnProperty(i.name)) {
            let oldI = oldByName[i.name];
            res.push({
                name: i.name,
                rs: Math.round((i.rs - oldI.rs) / timeDiff),
                rp: Math.round((i.rp - oldI.rp) / timeDiff),
                ts: Math.round((i.ts - oldI.ts) / timeDiff),
                tp: Math.round((i.tp - oldI.tp) / timeDiff)
            })
        } else {
            res.push({
                name: i.name,
                rs: 0,
                rp: 0,
                ts: 0,
                tp: 0
            });
        }
    }
    return res;
}


class Network extends Repeat {
    constructor(format) {
        super(500);
        this.formatter = new Formatter(formatRegexp, format);
        this.lastStats = null;
    }

    update() {
        return getNetworkStats().then(stats => {
            let usage = makeUsage(stats, this.lastStats);
            this.lastStats = stats;
            this.setText(this.formatter.format(match => {
                if (match.literal) return match.literal;
                let name = match.name ? match.name.replace(/\\./, str => str[1]) : "";
                let field = match.spec.toLowerCase();
                let target = usage.reduce(
                    (sum, stat) => sum + (stat.name.startsWith(name) ? stat[field] : 0),
                    0
                );
                switch (match.spec) {
                    case "rS":
                    case "rp":
                    case "tS":
                    case "tp":
                        return target.toString();

                    case "rs":
                    case "ts":
                        return formatSpeed(target);
                }
            }));
        });
    }
}


module.exports = Network;