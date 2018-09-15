const { Defer } = require("./util");


class Program {
    constructor(output) {
        this.output = null;
        this.sources = [];
        this.deferRefresh = new Defer();
    }

    setDeferTimeout(timeout) {
        this.deferRefresh.timeout = timeout;
        return this;
    }

    setOutput(output) {
        this.output = output;
        output.attach(this);
        this.refresh();
        return this;
    }

    addSource(source) {
        let i = this.sources.push(source) - 1;
        source.attach(() => this.refresh(i));
        return this;
    }

    configure(config) {
        for (let key in config) {
            if (config.hasOwnProperty(key)) {
                let value = config[key];
                switch (key) {
                    case "deferTimeout":
                        this.setDeferTimeout(value);
                        break

                    case "output":
                        this.setOutput(value);
                        break;

                    case "sources":
                        for (let source of value) {
                            this.addSource(source);
                        }
                        break;
                }
            }
        }
        return this;
    }

    click(source, button) {
        this.sources[source].click(button);
    }

    refresh() {
        this.deferRefresh.update(() => {
            this.output && this.output.display(this.sources.map(source => ({
                width: source._width,
                text: source._text
            })));
        });
    }
}

module.exports = Program;