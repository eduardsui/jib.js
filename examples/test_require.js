var fs = require('fs');

var readStream = fs.createReadStream("duktape/duktape.c");
var writeStream = fs.createWriteStream("test2.con");
readStream.on('open', function () {
    readStream.pipe(writeStream);
    console.log("here");
});

readStream.on('error', function(err) {
    writeStream.end(err);
});
