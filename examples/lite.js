if (global.esp) {
if (nvs.init()) { nvs.erase(); nvs.init(); }
console.log('esp memory available', esp.freeHeapSize(), esp.freeMinimumHeapSize());
console.log(wifi.strerror(wifi.stopDhcpc(wifi.constants.TCPIP_ADAPTER_IF_STA)));
console.log(wifi.strerror(wifi.stopDhcps(wifi.constants.TCPIP_ADAPTER_IF_AP)));
console.log(wifi.strerror(wifi.setIp(wifi.constants.TCPIP_ADAPTER_IF_AP)));
console.log(wifi.strerror(wifi.startDhcps(wifi.constants.TCPIP_ADAPTER_IF_AP)));
wifi.setMode(wifi.constants.WIFI_MODE_AP);
wifi.onip = function() { console.log('connected', wifi.getIp()); };
wifi.onstart = function() { console.log('started'); wifi.connect(); };
wifi.ondisconnect = function() { console.log('disconnected'); wifi.connect(); };
wifi.setConfig(wifi.constants.ESP_IF_WIFI_AP, { });
wifi.start();
}
if (global.esp)
console.log('esp memory available', esp.freeHeapSize(), esp.freeMinimumHeapSize());

const http = require('http');

if (global.esp)
console.log('esp memory available', esp.freeHeapSize(), esp.freeMinimumHeapSize());

const hostname = '0.0.0.0';
const port = 8080;

const template1 = '<html><head><meta name=\'viewport\' content=\'width=device-width, initial-scale=1,maximum-scale=1\'><meta http-equiv=\'Content-Type\' content=\'text/html;charset=UTF-8\'/><meta charset=utf-8/><title>Code editor</title><style>textarea {font-size: 16px;padding-left: 35px;padding-top: 10px;border-color:#ccc;width: 100%;height: 24010px;border: none;font-family: Courier New, Courier, Lucida Sans Typewriter, Lucida Typewriter, monospace;}body {padding: 0px;margin: 0px;}#header {width: 100%;background: black;display: block;border-bottom: 2px solid olive;padding: 10px;color: white;font-family: Arial, Helvetica, sans-serif;}#error {width: 100%;background: black;display: block;border-bottom: 2px solid olive;padding: 10px;color: white;font-family: Arial, Helvetica, sans-serif;}#run {border: 1px solid red;background: white;padding: 5px 30px 5px 30px;font-family: Arial, Helvetica, sans-serif;border-radius: 15px;}#container {overflow: auto;height: 100%;}pre {background-color: #FFE4B5;font-size: 16px;margin: 0px;padding: 20px;border-bottom: 1px solid olive;}body, html {height: 100%;overflow: hidden;}</style></head><body><form method = \'post\'><div id=\'header\'><input id=\'run\' type=\'submit\' value=\'&#9654; Run\'></div>';
const template2 = '<div id=\'container\'><textarea name=\'code\' onkeydown=\'if(event.keyCode===9){var v=this.value,s=this.selectionStart,e=this.selectionEnd;this.value=v.substring(0, s)+\'\\t\'+v.substring(e);this.selectionStart=this.selectionEnd=s+1;return false;}\'>';
const template3 = '</textarea></div></form></body></html>';

if (global.esp)
	console.log('esp memory available', esp.freeHeapSize(), esp.freeMinimumHeapSize());

const default_code = 'console.log(\'hello world\');\n';

const server = http.createServer(function(req, res) {
	console.log(server.connections, 'connections');

	res.statusCode = 200;
	res.setHeader('Connection', 'keep-alive');
	res.setHeader('Content-type', 'text/html');

	if (req.url != '/') {
		res.statusCode = 404;
		res.end('not found');
		return;
	}
	if (req.method == 'POST'){ 
		var buffer;
		req.on('data', function(data) {
			if (!buffer)
				buffer = data;
			else
				buffer = buffer + data;
		});
		req.on('end', function() {
			var str = '';
			var error = '';
			try {
				if (buffer) {
					for (var i = 0; i < buffer.length; i ++) {
						if (buffer[i] == 0x2b)
							buffer[i] = 0x20;
					}

					str = decodeURIComponent(new TextDecoder('utf-8').decode(buffer.subarray(5)));
				}

				var val = eval(str);
				if (val)
					error = '<pre>returned ' + val.toString() + '</pre>';
			} catch (e) {
				console.log(e);
				error = '<pre>' + e.toString() + '</pre>';
			}
			res.write(template1);
			res.write(error);
			res.write(template2);
			res.write(str);
			res.end(template3);
		});
	} else {
		res.write(template1);
		res.write(template2);
		res.write(default_code);
		res.end(template3);
	}
});

server.listen(port, hostname, function() {
  console.log('Server running at http://' + hostname + ':' + port);
});
