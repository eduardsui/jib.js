const http = require('http');
const fs = require('fs');
const URL = require('url');

const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer(function(req, res) {
	console.log(server.connections, "connections");

	res.statusCode = 200;
	// res.setHeader('Content-Type', 'text/plain');
	res.setHeader('Connection', 'keep-alive');

	req.on('data', function(data) {
		console.log(data);
		try {
			console.log(new TextDecoder("utf-8").decode(data));
		} catch (e) {
			console.log(e);
		}
	});

	var url = URL(req.url);
	var pathname = url.pathname;
	if (!pathname)
		pathname = "/";
	if (pathname == "/")
		pathname += "index.html";

	
	var readStream = fs.createReadStream("." + pathname);
	readStream.on('open', function () {
		readStream.pipe(res);
	});

	readStream.on('error', function(err) {
		res.statusCode = 404;
		res.end(err.toString());
	});
});

server.listen(port, hostname, function() {
  console.log('Server running at http://' + hostname + ':' + port);
});
