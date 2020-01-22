var net = require('net');
var util = require('util');
var Stream = require('stream').Stream;
var EventEmitter = require('events').EventEmitter;

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
	ClientRequest: function(header, host, port, timeout, callback) {
		var self = this;
		var _headers_set = false;
		this.headers = { };
		this.statusCode = 0;
		this.httpVersion = 1.1;
		this.contentLength = 0;
		this.bytesRead = 0;
		this.aborted = false;

		this.socket = net.createConnection({ "host": host, "port": port}, function(err) {
			if (err) {
				self.emit('error', err);
				self.close();
			} else {
				self.socket.write(header);
				self.socket.ref();
			}
		});
		if (timeout)
			this.socket.setTimeout(timeout);

		this.socket.on('data', function(buf) {
			if (_headers_set) {
				self.bytesRead += buf.length;
				if (self.decoder)
					buf = self.decoder.decode(buf);
				self.emit("data", buf);
				if ((self.contentLength) && (self.bytesRead >= self.contentLength))
					self.close();
			} else {
				if ((self._header_buf) && (self._header_buf.length))
					buf = _concatenateUint8Arrays(self._header_buf, buf);
				var limit = buf.length - 3;
				var header_pos = 0;
				for (var i = 0; i < limit; i ++) {
					if ((buf[i] == 0x0D) && (buf[i + 1] == 0x0A) && (buf[i + 2] == 0x0D) && (buf[i + 3] == 0x0A)) {
						header_pos = i;
						_headers_set = true;
						break;
					}
				}
				var response = undefined;
				var pending_buffer;
				if (header_pos) {
					header_pos += 4;
					if (header_pos == buf.byteLength)
						response = _http_helpers.parseResponse(new TextDecoder("utf-8").decode(buf));
					else
						response = _http_helpers.parseResponse(new TextDecoder("utf-8").decode(buf.subarray(0, header_pos)));
					if (response) {
						self.statusCode = response.statusCode;
						self.headers = response.headers;
						self.httpVersion = response.httpVersion;
						self.contentLength = response.contentLenght;
						callback(self);
					}
					if (header_pos < buf.length)
						pending_buffer = buf.subarray(header_pos);
					delete self._header_buf;
				} else
				if (buf.length < 0x10000) {
					self._header_buf = buf;
					return;
				} else
					console.warn("header too big");
				if (pending_buffer) {
					self.bytesRead += pending_buffer.length;
					if (self.decoder)
						pending_buffer = self.decoder.decode(pending_buffer);
					self.emit("data", pending_buffer);
					if ((self.contentLength) && (self.bytesRead >= self.contentLength))
						self.close();
				}
			}
		});

		this.setEncoding = function(encoding) {
			if (encoding)
				this.decoder = new TextDecoder(encoding);
			else
				delete this.decoder;
			return this;
		}
		var _end_emitted = false;
		this.socket.on('error', function(err) { self.emit('error', err); self.close(); });
		this.socket.on('end', function(data) { if (_end_emitted) return; _end_emitted = true; self.emit('end', data); self.close(); });
		this.socket.on('close', function() { self.close(true); });

		this.write = function(data, encoding, callback) {
			return this.socket.write(data, encoding, callback);
		}

		this.end = function(data, encoding, callback) {
			return this.socket.end(data, encoding, callback);
		}

		this.close = function(skip_socket) {
			if ((this.bytesRead) && (!_end_emitted)) {
				_end_emitted = true;
				this.emit("end");
			}
			this.emit("close");
			if (this.socket) {
				this.socket.unref();
				if (!skip_socket)
					this.socket.close();
				delete this.socket;
			}
		}

		this.abort = function() {
			if (this.aborted)
				return;
			this._end_emitted = true;
			this.aborted = true;
			this.emit("abort");
			this.close();
		}
	},

	request: function(url_str, options, callback) {
		if (!callback) {
			callback = options;
			options = { };
		}
		if (typeof url_str == "object") {
			options = url_str;
			url_str = undefined;
		}
		if (!options)
			options = { };


		var method = options.method ? options.method.toUpperCase() : "GET";
		var agent = options.agent ? options.agent : "jib.js";
		var host = options.host ? options.host : "";
		var timeout = options.timeout ? options.timeout : 3000;
		var port = options.port ? options.port : 80;
		var path = options.path ? options.path : "";
		var protocol = options.protocol ? options.protocol : "http";
		var headers = options.headers ? options.headers : undefined;
		var content = options.content ? options.content : undefined;

		if (!headers) {
			headers = {"Connection": "close"};
		} else
		if (!headers["Connection"])
			headers["Connection"] = "close";

		if (url_str) {
			var Url = require('url'); 
			var url = new Url(url_str);
			protocol = ((url.protocol) && (url.protocol.length)) ? url.protocol : protocol;
			host = ((url.hostname) && (url.hostname.length)) ? url.hostname : host;
			port = url.port ? url.port : port;
			path = ((url.pathname) && (url.pathname.length)) ? url.pathname : path;
			if ((url.query) && (url.query.length))
				path += "?" + url.query;
		}

		if ((!path) || (!path.length))
			path = "/";

		var host_str = host;
		if (port != 80)
			host_str += ":" + port;
		var req_str = "" + method + " " + encodeURI(path) + " HTTP/1.1\r\nHost: " + host_str + "\r\n";
		if ((agent) && (agent.length))
			req_str += "User-Agent: " + agent + "\r\n";
		if (headers) {
			for (var k in headers)
				req_str += k + ": " + headers[k] + "\r\n";
		}
		req_str += "\r\n";
		if ((content) && (content.length))
			req_str += content;

		return new http.ClientRequest(req_str, host, port, timeout, callback);
	},

	get: function(url, options, callback) {
		return http.request(url, options, callback);
	},

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
		this.connectionClose = true;
		this._readTimeout = this.socket._readTimeout;
		
		this._errorHandler = function(err) { this.end(); }
		this._closeHandler = function(arg) { this.emit("end", arg); }

		this.setTimeout = function(timeout) {
			this.socket.setTimeout(timeout);
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
					this.socket.setTimeout(this._readTimeout);
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
				if (this.connectionClose) {
					this.socket.close();
					this.socket = undefined;
				}
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
			c.setTimeout(3000);
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
						c._header_buf = undefined;
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
							var response = new http.ServerResponse(c);
							if ((request) && (request.Connection) && (request.Connection === "keep-alive"))
								response.connectionClose = false;
							requestListener(message, response);
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
									pending_buffer = undefined;
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
util.inherits(http.ClientRequest, EventEmitter);
util.inherits(http.ClientRequest, Stream.Stream);
