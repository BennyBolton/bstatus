const Repeat = require("./repeat");
const XRegExp = require("xregexp");
const {
    Formatter, formatSize, formatPortion, resultCache, exec
} = require("../util");


const parseDfOutputRegexp = XRegExp(`
    \\s+ (?<total> \\d+)
    \\s+ (?<used> \\d+)
    \\s+ (?<available> \\d+)
    \\s+ \\d+%
    \\s+ (?<path> /[^\\s]*)
`, "x");

const formatRegexp = XRegExp(`
    % (?<literal> %) |
    % (\\((?<location> ([^\\\)]+|\\.)*)\\))?
        (?<spec> [tuaTUApP])
`, "x");


function getDiskUsageStats() {
    return resultCache("Source.DiskUsage", () =>
        exec("df").then(data => {
            let res = {};
            for (let line of data.split("\n")) {
                let match = XRegExp.exec(line, parseDfOutputRegexp);
                if (match) {
                    res[match.path] = {
                        total: +match.total * 1024,
                        used: +match.used * 1024,
                        available: +match.available * 1024,
                    };
                }
            }
            return res;
        })
    );
}


function lookupLocation(stats, location) {
    while (location.length > 0) {
        if (stats.hasOwnProperty(location)) {
            return stats[location];
        }
        let i = location.lastIndexOf("/");
        location = i < 0 ? "" : location.substring(0, i);
    }
    return { total: 0, used: 0, available: 0 };
}


class DiskUsage extends Repeat {
    constructor(format) {
        super(10000);
        this.formatter = new Formatter(formatRegexp, format);
    }

    update() {
        return getDiskUsageStats().then(stats => {
            this.setText(this.formatter.format(match => {
                if (match.literal) return match.literal;

                let target = lookupLocation(stats, match.location || "/");
                switch (match.spec) {
                    case "T": return target.total.toString();
                    case "U": return target.used.toString();
                    case "A": return target.available.toString();

                    case "t": return formatSize(target.total);
                    case "u": return formatSize(target.used);
                    case "a": return formatSize(target.available);

                    case "p": return formatPortion(target.used / target.total);
                    case "P": return formatPortion(target.available / target.total);
                }
            }));
        });
    }
}


module.exports = DiskUsage;