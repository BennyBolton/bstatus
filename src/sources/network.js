const Repeat = require("./repeat");
const fs = require("fs");
const { alignTimeout } = require("../util");


/*
    TODO:

    - Conditionals
*/


const parseNeworkStatsRegexp = /^\s*([^:]+):\s*(\d+)\s+(\d+)(\s+\d+){6}\s+(\d+)\s+(\d+)\s/;
const formatRegexp = /%%|%(\((([^\\\)]+|\\.)*)\))?([rt][sSp])/g;
const parseFormatRegexp = /^%(\((.*)\))?(%|[rt][sSp])$/;
const prefixes = ["B/s", "kB/s", "MB/s", "GB/s", "TB/s", "PB/s", "EB/s", "ZB/s", "YB/s"];


function getNetworkStats() {
    return new Promise((resolve, reject) => {
        fs.readFile("/proc/net/dev", (err, data) => {
            if (err) {
                reject(err);
            } else {
                let res = { time: Date.now(), stats: [] };
                try {
                    for (let line of data.toString().split("\n")) {
                        let match = line.match(parseNeworkStatsRegexp);
                        if (match) {
                            res.stats.push({
                                name: match[1],
                                receiveBytes: +match[2],
                                receivePackets: +match[3],
                                transmitBytes: +match[5],
                                transmitPackets: +match[6]
                            });
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
                receiveBytes: Math.round((i.receiveBytes - oldI.receiveBytes) / timeDiff),
                receivePackets: Math.round((i.receivePackets - oldI.receivePackets) / timeDiff),
                transmitBytes: Math.round((i.transmitBytes - oldI.transmitBytes) / timeDiff),
                transmitPackets: Math.round((i.transmitPackets - oldI.transmitPackets) / timeDiff)
            })
        } else {
            res.push({
                name: i.name,
                receiveBytes: 0,
                receivePackets: 0,
                transmitBytes: 0,
                transmitPackets: 0
            });
        }
    }
    return res;
}


function formatSize(size) {
    let prefix = 0;
    size = size * 10;
    while (size >= 10000) {
        size = Math.round(size / 1000);
        ++prefix;
    }
    return `${Math.floor(size / 10)}.${size % 10} ${prefixes[prefix]}`;
}


function formatSpecifier(usage, fake) {
    function getField(name, field) {
        return usage.reduce((sum, stat) => {
            if (!name || stat.name.startsWith(name)) {
                sum += stat[field];
            }
            return sum;
        }, 0);
    }

    return str => {
        let match = str.match(parseFormatRegexp);
        if (!match) return str;
        if (fake) return match[3][1] == "s" ? "xxx.x xx/s" : "x";

        let name = match[2] && match[2].replace(/\\./, str => str[1]);
        switch (match[3]) {
            case "%": return "%";

            case "rs": return formatSize(getField(name, "receiveBytes"));
            case "rS": return getField(name, "receiveBytes").toString();
            case "rp": return getField(name, "receivePackets").toString();
            case "ts": return formatSize(getField(name, "transmitBytes"));
            case "tS": return getField(name, "transmitBytes").toString();
            case "tp": return getField(name, "transmitPackets").toString();
        }
        throw new Error("Network.update: Internal error: Bad format: " + str);
    };
}


class Network extends Repeat {
    constructor(format) {
        super(500);
        this.format = format;
        this.lastStats = null;
    }

    update() {
        return getNetworkStats().then(stats => {
            let usage = makeUsage(stats, this.lastStats);
            this.lastStats = stats;
            this.setWidth(this.format.replace(formatRegexp, formatSpecifier(usage, true)));
            this.setText(this.format.replace(formatRegexp, formatSpecifier(usage)));
        });
    }
}


module.exports = Network;