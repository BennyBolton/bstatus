const Program = require("./program");
const { resolve } = require("path");
const { existsSync } = require("fs");


let global = (function () { return this; })();
let program = new Program();

global.Util = require("./util");
global.Format = require("./format");
global.Output = require("./output");
global.Source = require("./source");

global.Output.Standard = require("./outputs/standard");
global.Output.I3Bar = require("./outputs/i3bar");

global.Source.Clock = require("./sources/clock");
global.Source.Cpu = require("./sources/cpu");
global.Source.DiskUsage = require("./sources/diskusage");
global.Source.Memory = require("./sources/memory");
global.Source.Network = require("./sources/network");
global.Source.Repeat = require("./sources/repeat");

global.configure = config => program.configure(config);


let configFile = [
    process.env["HOME"] + "/.config/bstatus/config.js",
    process.env["HOME"] + "/.bstatus/config.js"
].find(existsSync);

if (process.argv.length > 2) {
    if (process.argv.length > 3 || process.argv[2] == "--help") {
        console.error(`Usage: bstatus [config-file]`);
        process.exit(-1);
    }
    configFile = resolve(process.cwd(), process.argv[2]);
} else if (configFile === undefined) {
    console.error("Could not find config file");
    process.exit(-1);
}

if (typeof __webpack_require__ === "function") {
    __non_webpack_require__(configFile);
} else {
    require(configFile);
}
