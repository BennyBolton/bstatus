const Repeat = require("./repeat");
const { alignTimeout } = require("../util");
const strftime = require("strftime");


class Clock extends Repeat {
    constructor(format) {
        super(1000);
        this.format = format;
    }

    update() {
        this.setText(strftime(this.format, new Date()));
    }
}


module.exports = Clock;