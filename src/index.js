const Program = require("./program");
const { resolve } = require("path");


let global = (function () { return this; })();
let program = new Program();

global.Util = require("./util");
global.Output = require("./output");
global.Source = require("./source");

global.Output.Standard = require("./outputs/standard");
global.Output.I3Bar = require("./outputs/i3bar");

global.Source.Clock = require("./sources/clock");
global.Source.Cpu = require("./sources/cpu");
global.Source.Memory = require("./sources/memory");
global.Source.Network = require("./sources/network");
global.Source.Repeat = require("./sources/repeat");

global.configure = config => program.configure(config);


let configFile = process.env["HOME"] + "/.bstatus/config.js";
if (process.argv.length > 2) {
    if (process.argv.length > 3 || process.argv[2] == "--help") {
        console.error(`Usage: bstatus [config-file]`);
        process.exit(-1);
    }
    configFile = resolve(process.cwd(), process.argv[2]);
}

if (typeof __webpack_require__ === "function") {
    __non_webpack_require__(configFile);
} else {
    require(configFile);
}