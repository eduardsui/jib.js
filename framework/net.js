var EventEmitter = require('events').EventEmitter;
var util = require('util');
var Stream = require('stream').Stream;

var net = {
	constants: {
		SOCK_STREAM: 1,
		SOCK_DGRAM: 2,
		SOCK_RAW: 3,
		SOCK_RDM: 4,
		SOCK_SEQPACKET: 5,
		SOCK_PACKET: 10,

		AF_UNSPEC: 0,
		AF_UNIX: 1,
		AF_INET: 2,
		AF_AX25: 3,
		AF_IPX: 4,
		AF_APPLETALK: 5,
		AF_NETROM: 6,
		AF_BRIDGE: 7,
		AF_AAL5: 8,
		AF_X25: 9,
		AF_INET6: 10,
		AF_MAX: 12
	},

	Socket: function(options, connectListener) {
		if (!options)
			options = { };

		Stream.Duplex.call(this, options);

		this._socket = undefined;
		this.bytesRead = 0;
		this.bytesWritten = 0;
		this.connecting = false;
		this.destroyed = false;
		this._drain = false;
		this._buffer = undefined;
		this._half_closed = false;
		this._info = null;
		this.bufferSize = 0;
		this._lastRead = new Date().getTime();

		var readable = true;
		var writable = true;

		var self = this;
		var server = undefined;

		if (options) {
			this._socket = options.fd;
			if (options.readable !== undefined)
				readable = options.readable;
			if (options.writable !== undefined)
				writable = options.writable;
			server = options._server;
		}

		if (this._socket == undefined) {
			var host = options.host ? options.host : "localhost";
			var port = options.port;
			var connected = false;

			this._socket = _net_.socket(net.constants.AF_INET, net.constants.SOCK_STREAM, 0);
			if (this._socket < 0) {
				this.emit("error", new Error(_net_.strerror(_net_.errno())));
			} else
			if (_net_.connect(this._socket, host, port)) {
				this.emit("error", new Error(_net_.strerror(_net_.errno())));
			} else {
				connected = true;
				this.emit("lookup");
				this.emit("connect");
				this.emit("ready");
			}

			if (connectListener)
				setImmediate(function() { connectListener(connected ? undefined : new Error(_net_.strerror(_net_.errno()))); });
		}
		if (this._socket !== undefined) {
			var read_write = 0;
			var obj = { };
			if (readable) {
				obj.read = function() {
					var buf = new Uint8Array(8192);
					var nbytes = _net_.recv(self._socket, buf, buf.byteLength);
					if (nbytes < 0) {
						self.emit("error", new Error(_net_.strerror(_net_.errno())));
					} else
					if (nbytes) {
						self._lastRead = new Date().getTime();
						self.bytesRead += nbytes;
						if (nbytes != 8192)
							buf = buf.subarray(0, nbytes);
						self.emit("data", buf);
					} else {
						self.close();
					}
				};
			}
			if (writable) {
				read_write = 1;
				obj.write = function() {
					if (self.destroyed)
						return;
					self._drain = true;
					self.emit("drain");
					if ((!self._buffer) || (!self._buffer.length))
						_net_.pollPauseWrite(self._socket);
					self.flush();
				};
			}
			_net_.poll(this._socket, read_write, obj);
		}

		this.address = function() {
			if (!this._info)
				this._info = _net_.info(this._socket);
			if (this._info)
				return {"port": this._info["local_port"], "family": this._info["local_family"] === net.constants.AF_INET6 ? "IPv6" : "IPv4" , "address": this._info["local_address"]};
		}

		Object.defineProperty(this, "localAddress", {  get: function() { if (!this._info) this._info = _net_.info(this._socket); if (this._info) return this._info["local_address"]; } } );
		Object.defineProperty(this, "localPort", {  get: function() { if (!this._info) this._info = _net_.info(this._socket); if (this._info) return this._info["local_port"]; } } );
		Object.defineProperty(this, "remoteAddress", {  get: function() { if (!this._info) this._info = _net_.info(this._socket); if (this._info) return this._info["remote_address"]; } } );
		Object.defineProperty(this, "remoteFamily", {  get: function() { if (!this._info) this._info = _net_.info(this._socket); if (this._info) return this._info["remote_family"] === net.constants.AF_INET6 ? "IPv6" : "IPv4"; } } );
		Object.defineProperty(this, "remotePort", {  get: function() { if (!this._info) this._info = _net_.info(this._socket); if (this._info) return this._info["remote_port"]; } } );

		this.setNoDelay = function(noDelay) {
			if (noDelay === undefined)
				noDelay = true;
			if (_net_.setNoDelay(this._socket, noDelay))
				self.emit("error", new Error(_net_.strerror(_net_.errno())));

			return this;
		}

		this.setKeepAlive = function(enabled) {
			if (enabled === undefined)
				enabled = true;
			if (_net_.setKeepAlive(this._socket, enabled))
				self.emit("error", new Error(_net_.strerror(_net_.errno())));

			return this;
		}

		this.setTimeout = function(timeout, callback) {
			var err = _net_.setTimeout(this._socket, timeout);
			if (callback)
				setImmediate(function() { callback(err ? new Error(_net_.strerror(_net_.errno())) : undefined); });
			if (err)
				self.emit("error", new Error(_net_.strerror(_net_.errno())));
			return this;
		}

		this.setReadTimeout = function(timeout) {
			this._readTimeout = timeout;
			if (this._lastReadInterval)
			 	clearInterval(this._lastReadInterval);

			if (timeout > 0) {
				this._lastReadInterval = setInterval(function() {
					if (new Date().getTime() - self._lastRead > timeout) {
						clearInterval(self._lastReadInterval);
						self.close();
					}
				}, timeout / 2);
			}
			return this;
		}

		this.flush = function() {
			if ((this._drain) && (this._socket !== undefined) && (!this.destroyed) && (this._buffer) && (this._buffer.length)) {
				_net_.pollResumeWrite(this._socket);
				var sent = _net_.send(this._socket, this._buffer, this._buffer.length);
				if (sent > 0) {
					this._lastRead = new Date().getTime();
					this.bytesWritten += sent;
					if (sent == this._buffer.length) {
						this._buffer = undefined;
						if (this._half_closed) {
							this._half_closed = false;
							this.close();
						}
					} else
						this._buffer = this._buffer.slice(sent);
				} else
				if (sent < 0) {
					self.emit("error", new Error(_net_.strerror(_net_.errno())));
					this.close();
				}
			}
			return false;
		}

		this.write = function(data, encoding, callback) {
			if ((this.destroyed) || (this._socket === undefined))
				throw new Error("socket is closed");

			var buf = data;
			if (typeof data === "string") {
				if (!encoding)
					encoding = "utf-8";

				buf = new TextEncoder(encoding).encode(data);
			}
			if (this._buffer) {
				var buf2 = new Uint8Array(this._buffer.length + buf.length);
				buf2.set(this._buffer);
				buf2.set(buf, this._buffer.length);
				this._buffer = buf2;
			} else
				this._buffer = buf;
			return this.flush();
		}

		this._read = function(n) {
			return 0;
		}

		this.close = function() {
			if ((this._socket !== undefined) && (this._socket >= 0)) {
				this.emit("close");
				_net_.unpoll(this._socket);
				_net_.close(this._socket);
				this._socket = undefined;
				this._buffer = undefined;
				if (server) {
					server.connections --;
					server = undefined;
				}
				if (this._lastReadInterval) {
					clearInterval(this._lastReadInterval);
					this._lastReadInterval = undefined;
				}
				for (var k in this)
					delete this[k];
				this.destroyed = true;
			}
			this.removeAllListeners();
		}

		this.end = function(data, encoding, callback) {
			if (data)
				this.write(data, encoding, callback);

			this.emit("end", false);

			if (this._buffer) {
				this._half_closed = true;
				return false;
			}
			this.close();
			return true;
		}

		this.destroy = function() {
			this.destroyed = true;
			this.close();
		}

		this.finalize = function(self) {
			if ((self._socket !== undefined) && (self._socket >= 0))
				self.close();
		}

		this.__refHandler = undefined;

		this.ref = function() {
			if (!this.__refHandler)
				this.__refHandler = new Array();

			this.__refHandler.push(setInterval(function() { }, 3600000));
		}

		this.unref = function() {
			if ((this.__refHandler) && (this.__refHandler.length)) {
				var handler = this.__refHandler.pop();
				if (handler)
					clearInterval(handler);
			}
		}

		global.__destructor(this, this.finalize);
	},

	Server: function(connectionListener) {
		this.listening = false;
		this.maxConnections = 0;
		this.connections = 0;
		this.connectionListener = connectionListener;
		this._info = undefined;

		this._socket = _net_.socket(net.constants.AF_INET, net.constants.SOCK_STREAM, 0);

		this.listen = function(port, host, backlog, callback) {				
			if (!callback) {
				callback = backlog;
				backlog = 1024;
			}
			if (!callback) {
				callback = host;
				host = "0.0.0.0";
			}
			if (!host)
				host = "0.0.0.0";
			
			if (this.listening) {
				if (callback)
					callback(new Error("already listening"));
				this.emit("error", new Error("already listening"));
				return this;
			}

			var ipv6_only = false;
			if (typeof port == "object") {
				var options = port;
				host = options.host ? options.host : "0.0.0.0";
				port = options.port;
				backlog = options.backlog ? options.backlog : 1024;
				ipv6_only = options.ipv6Only ? options.ipv6Only : false;
				if (!options.exclusive)
					_net_.setReuseAddr(this._socket, true);
			}

			this.listening = true;
			_net_.bind(this._socket, host, port)
			var self = this;
			if (_net_.listen(this._socket, backlog)) {
				if (callback)
					setImmediate(function() { callback(new Error(_net_.strerror(_net_.errno()))); });
				this.emit("error", new Error(_net_.strerror(_net_.errno())));
			} else {
				this.ref();
				_net_.poll(this._socket, 0, {"read": function() { 
					var sock = _net_.accept(self._socket);
					if (sock < 0) {
						self.emit("error", new Error(_net_.strerror(_net_.errno())));
					} else {
						_net_.setKeepAlive(sock, true);
						var sock_obj = new net.Socket({ "fd": sock, "readable": true, "writable": true, "_server": self });
						self.emit("connection", sock_obj);
						self.connections ++;

						if (self.connectionListener)
							self.connectionListener(sock_obj);
					}
				}});
				if (callback)
					setImmediate(callback);
				this.emit("listening");
			}			
			return this;
		}

		this.close = function() {
			if ((this._socket !== undefined) && (this._socket >= 0)) {
				this.emit("close");
				_net_.unpoll(this._socket);
				_net_.close(this._socket);
				this._socket = undefined;
			}
			this.unref();
			this.removeAllListeners();
		}

		this.getConnections = function(callback) {
			var self = this;
			setImmediate(callback(undefined, self.connections));
		}

		this.address = function() {
			if (!this._info)
				this._info = _net_.info(this._socket);
			if (this._info)
				return {"port": this._info["local_port"], "family": this._info["local_family"] === net.constants.AF_INET6 ? "IPv6" : "IPv4" , "address": this._info["local_address"]};
		}

		this.__refHandler = undefined;

		this.ref = function() {
			if (!this.__refHandler)
				this.__refHandler = new Array();

			this.__refHandler.push(setInterval(function() { }, 3600000));
		}

		this.unref = function() {
			if ((this.__refHandler) && (this.__refHandler.length)) {
				var handler = this.__refHandler.pop();
				if (handler)
					clearInterval(handler);
			}
		}

		this.finalize = function(self) {
			self.close();
		}

		global.__destructor(this, this.finalize);
	},

	createServer(options, connectionListener) {
		if (connectionListener === undefined)
			connectionListener = options;
		return new this.Server(connectionListener);
	},

	createConnection(options, connectionListener) {
		return new this.Socket(options, connectionListener);
	},

	connect(options, connectionListener) {
		return createConnection(options, connectionListener);
	}
}

util.inherits(net.Socket, Stream.Duplex);
util.inherits(net.Server, EventEmitter);

module.exports = net;
