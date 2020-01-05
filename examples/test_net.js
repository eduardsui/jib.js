var net = require("net");
var fs = require('fs');

const server = net.createServer(function(c) {
	try {
		console.log('client connected #' + server.connections, c.remoteAddress, c.remotePort);
		c.on('data', function(buf) {
console.log(console.stringify(_http_helpers.parseRequest(new TextDecoder("utf-8").decode(buf))));
			console.log(new TextDecoder("utf-8").decode(buf));
		});

		c.on('end', function() {
			console.log('client disconnected');
		});

		var readStream = fs.createReadStream("test_net.js");
		readStream.on('open', function () {
			c.write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n");
			readStream.pipe(c);
		});
	} catch (e) {
		console.error(e);
	}
});

server.on('error', function(err) {
	throw err;
});

server.listen(80, function() {
	console.log('server bound');
});
