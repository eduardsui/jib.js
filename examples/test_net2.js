const net = require('net');
const client = net.createConnection({ port: 80, host: "devronium.com" }, function() {
  // 'connect' listener.
  console.log('connected to server!');
  client.write('world!\r\n');
});
client.on('data', function(data) {
  console.log(new TextDecoder("utf-8").decode(data));
  client.end();
});
client.on('end', function() {
  console.log('disconnected from server');
  // client.unref();
});


process.on('SIGINT', function(sig) {
	console.log("CTRL-C");
	app.quit();
});

app.onexit = function() {
	console.log("exit");
}

client.ref();
