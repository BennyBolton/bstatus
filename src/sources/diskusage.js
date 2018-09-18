const Repeat = require("./repeat");
const XRegExp = require("xregexp");
const { resultCache, makeCondition, exec } = require("../util");
const { makeFormatter, formatSize, formatPortion } = require("../format");


const parseDfOutputRegexp = XRegExp(`
    \\s+ (?<total> \\d+)
    \\s+ (?<used> \\d+)
    \\s+ (?<available> \\d+)
    \\s+ \\d+%
    \\s+ (?<path> /[^\\s]*)
`, "x");


const formatRules = [
    {
        regex: /%%/,
        format: "%"
    },
    {
        regex: XRegExp(`%
            (\\((?<location> ([^\\\\\\)]+|\\.)*)\\))?
            (?<spec> [tuaTUApP])
        `, "x"),
        format(match) {
            let location = (match.location || "/").replace(/\\./, str => str[1]);
            let wrap = cb => usage => cb(lookupLocation(usage, location));
            switch (match.spec) {
                case "T": return wrap(target => target.total.toString());
                case "U": return wrap(target => target.used.toString());
                case "A": return wrap(target => target.available.toString());

                case "t": return wrap(target => formatSize(target.total));
                case "u": return wrap(target => formatSize(target.used));
                case "a": return wrap(target => formatSize(target.available));

                case "p":
                    return wrap(target =>
                        formatPortion(target.used / (target.total || 1))
                    );
                case "P":
                    return wrap(target =>
                        formatPortion(target.available / (target.total || 1))
                    );
            }
        }
    },
    {
        regex: XRegExp(`%
            ${makeCondition.regex}
            (\\((?<location> ([^\\\\\\)]+|\\.)*)\\))?
            (?<spec> [tuapP])
        `, "x"),
        format(match) {
            let condition = makeCondition(match);
            let location = (match.location || "/").replace(/\\./, str => str[1]);
            let value;
            switch (match.spec) {
                case "t": value = target => target.total; break;
                case "u": value = target => target.used; break;
                case "a": value = target => target.available; break;

                case "p":
                    value = target => 100 * target.used / (target.total || 1);
                    break;
                case "P":
                    value = target => 100 * target.available / (target.total || 1);
                    break;
            }
            return usage => condition(value(lookupLocation(usage, location)));
        }
    }
];


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
    return stats["/"] || { total: 0, used: 0, available: 0 };
}


class DiskUsage extends Repeat {
    constructor(format) {
        super(10000);
        this.formatter = makeFormatter(formatRules, format);
    }

    update() {
        return getDiskUsageStats().then(usage => {
            this.setText(this.formatter.format(usage));
        });
    }
}


module.exports = DiskUsage;