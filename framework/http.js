var net = require('net');
var util = require('util');
var Stream = require('stream').Stream;


function _concatenateUint8Arrays(header_buf, buf) {
	if (header_buf) {
		var new_buf = new Uint8Array(header_buf.length + buf.length);
		new_buf.set(header_buf);
		new_buf.set(buf, header_buf.length);
		return new_buf;
	}
	return buf;
}

var http = {
	Server: function(options) {
		net.Server.call(this, options); 
	},

	IncomingMessage: function(c, request) {
		this.url = request.url;
		this.method = request.method;
		this.headers = request.headers;
		this.upgrade = request.upgrade;
		this.httpVersion = request.httpVersion;
		this.socket = c;
	},

	ServerResponse: function(c, request) {
		this.socket = c;
		this._headers = { "Date": new Date().toUTCString(), "Transfer-Encoding": "chunked" };
		this.headersSent = false;
		this.statusCode = 200;
		this.statusMessage = "OK";
		this.writableEnded = false;
		this._readTimeout = this.socket._readTimeout;
		
		this._errorHandler = function(err) { this.end(); }
		this._closeHandler = function(arg) { this.emit("end", arg); }

		this.setTimeout = function(timeout) {
			this.socket.setReadTimeout(timeout);
			return this;
		}

		var self = this;
		this.socket.on("close", this._closeHandler);
		this.socket.on("error", this._errorHandler);

		this.setHeader = function(name, val) {
			this._headers[name] = val;
			return this;
		},

		this.getHeader = function(name) {
			return this._headers[name];
		},

		this.getHeaders = function() {
			return this._headers;
		},

		this.hasHeader = function(name) {
			return (this._headers[name] !== undefined);
		},

		this.removeHeader = function(name) {
			delete this._headers[name];
			return this;
		},

		this.write = function(data, encoding, callback) {
			if ((!this.socket) || (!data) || (!data.length))
				return this;
			try {
				this.flushHeaders();

				if (this._headers["Transfer-Encoding"] === "chunked") {
					if (typeof data === "string")
						this.socket.write("" + new TextEncoder("utf-8").encode(data).byteLength.toString(16) + "\r\n", "utf-8");
					else
						this.socket.write("" + data.byteLength.toString(16) + "\r\n", "utf-8");
				}
				this.socket.write(data, encoding, callback);
				this.socket.write("\r\n", "utf-8");
			} catch (e) {
				this.headersSent = true;
				this._buffer = undefined;
				this.end();
			}
			return this;
		},

		this.end = function(data, encoding, callback) {
			if (this.writableEnded)
				return this;
			this.writableEnded = true;
			try {
				this.flushHeaders();
				if ((this.socket) && (!this.socket.destroyed)) {
					if ((data) && (data.length))
						this.write(data, encoding, callback);
					if (this._headers["Transfer-Encoding"] === "chunked")
						this.socket.write("0\r\n\r\n", "utf-8");
					this.socket.setReadTimeout(this._readTimeout);
				}
			} catch (e) {
				console.warn(e.toString());
			}
			this.emit("finish");
			this.removeAllListeners();
			if (this.socket) {
				delete this.socket._processRequest;
				this.socket.removeListener("close", this._closeHandler);
				this.socket.removeListener("error", this._errorHandler);
			}
			return this;
		},

		this.writeHead = function(statusCode, statusMessage, headers) {
			if ((this.headersSent) || (!this.socket))
				return this;
			this.headersSent = true;

			if (!headers)
				headers = { };
			if (!statusMessage)
				statusMessage = "";

			var data = "HTTP/1.1 " + statusCode + " " + statusMessage + "\r\n";
			for (var k in headers)
				data += k + ": " + headers[k] + "\r\n";
			data += "\r\n";
			this.socket.write(data, "utf-8");
			return this;	
		},

		this.flushHeaders = function() {
			if (this.headersSent)
				return this;

			return this.writeHead(this.statusCode, this.statusMessage, this._headers);
		}
	},

	createServer: function(options, requestListener) {
		if ((!requestListener) && (typeof options !== "object")) {
			requestListener = options;
			options = undefined;
		}

		var gc_timestamp = Date.now();
		var connectionHandler = function(c) {
			c.setReadTimeout(3000);
			c.on('error', function(e) {
				console.log(e);
			});
			c.on('data', function(buf) {
				if ((c._processRequest) && (c._processRequest_remaining > 0)) {
					if (c._processRequest_remaining >= buf.length) {
						c._processRequest.emit("data", buf);
						c._processRequest_remaining -= buf.length;
						if (!c._processRequest_remaining) {
							c._processRequest.emit("end");
							if (c._processRequest)
								c._processRequest.removeAllListeners();
							delete c._processRequest_remaining;
							delete c._processRequest;
						}
						return;
					} else {
						c._processRequest.emit("data", buf.subarray(0, c._processRequest_remaining));
						c._processRequest.emit("end");
						if (c._processRequest)
							c._processRequest.removeAllListeners();
						delete c._processRequest_remaining;
						delete c._processRequest;

						buf = buf.subarray(c._processRequest_remaining);
					}

				}

				if (Date.now() - gc_timestamp >= 2000)
					global.gc();

				var pending_buffer;
				try {
					buf = _concatenateUint8Arrays(c._header_buf, buf);
					var limit = buf.length - 3;
					var header_pos = 0;
					for (var i = 0; i < limit; i ++) {
						if ((buf[i] == 0x0D) && (buf[i + 1] == 0x0A) && (buf[i + 2] == 0x0D) && (buf[i + 3] == 0x0A)) {
							header_pos = i;
							break;
						}
					}
					var request = undefined;
					if (header_pos) {
						header_pos += 4;
						if (header_pos == buf.byteLength)
							request = _http_helpers.parseRequest(new TextDecoder("utf-8").decode(buf));
						else
							request = _http_helpers.parseRequest(new TextDecoder("utf-8").decode(buf.subarray(0, header_pos)));
						if (header_pos < buf.length)
							pending_buffer = buf.subarray(header_pos);
						delete c._header_buf;
					} else
					if (buf.length < 0x10000) {
						c._header_buf = buf;
						return;
					} else
						console.warn("header too big");
				} catch (e) {
					request = undefined;
					console.log(e);
				}
				try {
					if (request) {
						if (requestListener) {
							var message = new http.IncomingMessage(c, request);
							requestListener(message, new http.ServerResponse(c));
							var remaining = request.contentLength;
							if (pending_buffer) {
								if (remaining > 0) {
									if (remaining <= pending_buffer.length) {
										message.emit("data", pending_buffer);
										remaining -= pending_buffer.length;
									} else {
										remaining = 0;
										message.emit("data", pending_buffer.subarray(0, remaining));
										c.emit("data", pending_buffer.subarray(request.contentLength));
									}
									delete pending_buffer;
								} else
									c.emit("data", pending_buffer);
							}
							if (remaining) {
								c._processRequest = message;
								c._processRequest_remaining = request.contentLength;
							} else {
								message.emit("end");
							}
						}
					} else
						c.close();
				} catch (e) {
					console.log(e);
					c.close();
				}
			});
		};
		return new http.Server(connectionHandler);
	}
};

module.exports = http;

util.inherits(http.Server, net.Server);
util.inherits(http.IncomingMessage, EventEmitter);
util.inherits(http.IncomingMessage, Stream.Stream);
util.inherits(http.ServerResponse, EventEmitter);
util.inherits(http.ServerResponse, Stream.Writable);
