const Source = require("../source");
const { alignTimeout } = require("../util");


class Repeat extends Source {
    constructor(delay, cb) {
        super();
        this._delay = delay;
        this._cb = cb;
        this._updateHandle = null;
        setTimeout(() => this.triggerUpdate(), 0);
    }

    update() {
        try {
            return Promise.resolve(this._cb.call(this));
        } catch (err) {
            return Promise.reject(err);
        }
    }

    triggerUpdate() {
        if (this._updateHandle !== null) {
            clearTimeout(this._updateHandle);
            this._updateHandle = null;
        }
        Promise.resolve(this.update()).then(
            val => {
                if (val !== undefined) {
                    this.setText(val);
                }
            },
            err => this.setError(err)
        ).then(() => this._updateHandle = alignTimeout(this._delay, () => {
            this._updateHandle = null;
            this.triggerUpdate();
        }));
    }
}


module.exports = Repeat;