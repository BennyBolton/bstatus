const Repeat = require("./repeat");
const fs = require("fs");
const { alignTimeout, exec } = require("../util");


/*
    TODO

    - Conditionals
*/


const parseCpuStatsRegexp = /^\s*cpu(\d*)((\s+\d+){10})\s*$/;
const formatRegexp = /%%|%\d*(\.\d+)?p|%\d+(-\d+)?(\.\d+)?c|%\d*(\.\d+)?(\((([^\\\)]+|\\.)*)\))?C/g;
const parseFormatRegexp = /^%(\d*)(-(\d+))?(\.(\d+))?(\((([^\\\)]+|\\.)*)\))?[pcC]$/;


function getCpuStats() {
    return new Promise((resolve, reject) => {
        fs.readFile("/proc/stat", (err, data) => {
            if (err) {
                reject(err);
            } else {
                let res = { time: Date.now(), count: 0 };
                try {
                    for (let line of data.toString().split("\n")) {
                        let match = line.match(parseCpuStatsRegexp);
                        if (match) {
                            let index = 0;
                            if (match[1].length > 0) {
                                index = +match[1] + 1;
                                if (index > res.count) res.count = index;
                            }
                            let values = match[2].match(/\d+/g);
                            res[index] = +values[0] + +values[2];
                        }
                    }
                    res[0] /= res.count;
                } catch (err) {
                    return reject(err);
                }
                resolve(res);
            }
        });
    });
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


function formatUsage(value, denom, acc) {
    return (value * (+denom || 100)).toFixed(+acc || 0);
}

function formatSpecifier(usage, fake) {
    return str => {
        let match;
        switch (str[str.length - 1]) {
            case "%": return "%";

            case "p":
                match = str.match(parseFormatRegexp) || [];
                return formatUsage(fake ? 1 : usage[0], match[1], match[5]);

            case "c":
                match = str.match(parseFormatRegexp) || [];
                return formatUsage(fake ? 1 : (usage[match[1]] || 0), match[3], match[5]);

            case "C":
                match = str.match(parseFormatRegexp) || [];
                let txt = "";
                let seperator = ", ";
                if (match[7] !== undefined) {
                    seperator = match[7]
                }
                for (let i = 0; i < usage.count; ++i) {
                    if (i > 0) txt += seperator;
                    txt += formatUsage(fake ? 1 : usage[i + 1], match[1], match[5]);
                }
                return txt;
        }
        throw new Error("Cpu.update: Internal error: Bad format: " + str);
    };
}


class Cpu extends Repeat {
    constructor(format) {
        super(500);
        this.format = format;
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
            this.setWidth(this.format.replace(formatRegexp, formatSpecifier(usage, true)));
            this.setText(this.format.replace(formatRegexp, formatSpecifier(usage)));
        });
    }
}


module.exports = Cpu;