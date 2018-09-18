const Repeat = require("./repeat");
const fs = require("fs");
const XRegExp = require("xregexp");
const { resultCache, makeCondition } = require("../util");
const { makeFormatter, formatSpeed, formatSize } = require("../format");


const parseNeworkStatsRegexp = XRegExp(`
    ^
    \\s* (?<name> [^:]+):
    \\s* (?<rb> \\d+)
    \\s+ (?<rp> \\d+)
    (\\s+\\d+){6}
    \\s+ (?<tb> \\d+)
    \\s+ (?<tp> \\d+)
    \\s
`, "x");

const formatRegexp = XRegExp(`
    % (?<literal> %) |
    % (\\((?<name> ([^\\\)]+|\\.)*)\\))?
        (?<spec> [rt][sSp])
`, "x");


const formatRules = [
    {
        regex: /%%/,
        format: "%"
    },
    {
        regex: XRegExp(`%
            (\\((?<name> ([^\\\\\\)]+|\\.)*)\\))?
            (?<spec> [rt][bBsSp])
        `, "x"),
        format(match) {
            let name = (match.name || "").replace(/\\./, str => str[1]);
            let target = (usage, field) => usage.reduce(
                (sum, stat) => sum + (stat.name.startsWith(name) ? stat[field] : 0),
                0
            );
            switch (match.spec) {
                case "rb": return usage => formatSize(target(usage, "rb"));
                case "tb": return usage => formatSize(target(usage, "tb"));
                case "rs": return usage => formatSpeed(target(usage, "rs"));
                case "ts": return usage => formatSpeed(target(usage, "ts"));

                case "rS": return usage => target(usage, "rs").toString();
                case "rB": return usage => target(usage, "rb").toString();
                case "rp": return usage => target(usage, "rp").toString();
                case "tS": return usage => target(usage, "ts").toString();
                case "tB": return usage => target(usage, "tb").toString();
                case "tp": return usage => target(usage, "tp").toString();
            }
        }
    },
    {
        regex: XRegExp(`%
            ${makeCondition.regex}
            (\\((?<name> ([^\\\\\\)]+|\\.)*)\\))?
            (?<spec> [rt][bsp])
        `, "x"),
        format(match) {
            let condition = makeCondition(match);
            let name = (match.name || "").replace(/\\./, str => str[1]);
            let field = match.spec;
            let target = usage => usage.reduce(
                (sum, stat) => sum + (stat.name.startsWith(name) ? stat[field] : 0),
                0
            );
            return usage => condition(target(usage));
        }
    }
];


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
                                    rb: +match.rb,
                                    rp: +match.tp,
                                    tb: +match.tb,
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
                rb: i.rb,
                rs: Math.round((i.rb - oldI.rb) / timeDiff),
                rp: i.rp,
                tb: i.tb,
                ts: Math.round((i.tb - oldI.tb) / timeDiff),
                tp: i.tp
            })
        } else {
            res.push({
                name: i.name,
                rb: i.rb,
                rs: 0,
                rp: i.rp,
                tb: i.tb,
                ts: 0,
                tp: i.tp
            });
        }
    }
    return res;
}


class Network extends Repeat {
    constructor(format) {
        super(500);
        this.formatter = makeFormatter(formatRules, format);
        this.lastStats = null;
    }

    update() {
        return getNetworkStats().then(stats => {
            let usage = makeUsage(stats, this.lastStats);
            this.lastStats = stats;
            this.setText(this.formatter.format(usage));
        });
    }
}


module.exports = Network;