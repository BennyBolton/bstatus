const webpack = require("webpack");


module.exports = {
    entry: __dirname + "/src/index.js",
    output: {
        path: __dirname,
        filename: "bstatus.js"
    },
    plugins: [
        new webpack.BannerPlugin({
            banner: "#!/usr/bin/env node", raw: true
        })
    ],
    target: "node",
    mode: "production"
}