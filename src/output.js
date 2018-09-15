class Output {
    attach(program) {
        this._program = program;
    }

    click(source, button) {
        this._program && this._program.click(source, button);
    }

    display(sources) {
        throw new Error(`${constructor.name}.display not implemented`);
    }
}


module.exports = Output;