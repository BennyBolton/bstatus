const child_process = require("child_process");


class Defer {
    constructor(timeout) {
        this.timeout = timeout || 0;
        this.action = null;
        this._update = () => {
            this.action = null;
            this.cb();
        }
    }

    update(cb) {
        this.cb = cb;
        if (this.action === null) {
            this.action = setTimeout(this._update, this.timeout);
        }
    }
}


function alignTimeout(delay, cb) {
    let time = Date.now() % delay;
    return setTimeout(cb, delay - time);
}


function parseColor(color) {
    let denom = 15, width = 1;
    if (color.length > 3) {
        denom = 255; width = 2;
    }
    return [0, 1, 2].map(i =>
        parseInt("0x" + color.substring(i * width, i * width + width)) / denom
    );
}


function exec(cmd) {
    return new Promise((resolve, reject) => {
        child_process.exec(cmd, (err, res) => {
            if (err) {
                reject(err);
            } else {
                resolve(res.trim());
            }
        });
    });
}


function fork(cmd) {
    child_process.spawn("nohup", ["nohup", "bash", "-c", cmd], { stdio: null });
}


module.exports = {
    Defer,
    alignTimeout,
    parseColor,
    exec,
    fork
};