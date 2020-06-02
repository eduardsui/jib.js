var EventEmitter = require('events').EventEmitter;
var util = require('util');
var net = require('net');

var _net_socket = net.Socket;
var _net_server = net.Server;

var tls = {
	constants: net.constants,

	Socket: function(options, connectListener) {
		var self = this;
		this.established = false;
		var server = undefined;
		var handshake_timeout = 12000;
		if (options)
			handshake_timeout = options.handshakeTimeout ? options.handshakeTimeout : handshake_timeout;

		if ((options) && (options.tls_context)) {
			server = options._server;
			this.tls_context = options.tls_context;
		} else {
			this.tls_context = _tls_.create(false);

			_net_socket.call(this, options, function(err) {
				if (err) {
					connectListener(err);
					self.close();
					return;
				}
			});
			_tls_.connect(this.tls_context);
		}
		_net_.unpoll(this._socket);

		var obj = { };
		setTimeout(function() {
			if ((self.tls_context) && (!_tls_.established(self.tls_context))) {
				self.emit("error", new Error("Handshake timed out"));
				self.close();
			}
		}, handshake_timeout);
		obj.read = function() {
			var buf = new Uint8Array(8192);
			var nbytes = _net_.recv(self._socket, buf, buf.byteLength);
			if (nbytes < 0) {
				self.emit("error", new Error(_net_.strerror(_net_.errno())));
			} else
			if (nbytes) {
				self.bytesRead += nbytes;
				var consumed = _tls_.consume(self.tls_context, buf, nbytes);
				if (consumed < 0)
					self.emit("error", new Error("Error " + consumed + " in tls socket"));
				if (!self.tls_context)
					return;
				self._lastRead = new Date().getTime();
				if (_tls_.established(self.tls_context)) {
					if (!self.established) {
						self.established = true;
						if (connectListener)
							connectListener(self);
					}
					do {
						if (!self.tls_context)
							break;
						nbytes = _tls_.read(self.tls_context, buf, buf.byteLength);
						if (nbytes < 0) {
							self.emit("error", new Error(_C_library.strerror(_C_library.errno())));
							break;
						}
						if (nbytes == 0)
							break;
						var buf2 = buf;
						if (nbytes != 8192)
							buf2 = buf2.subarray(0, nbytes);
						if ((self._readableState) && (self._readableState.decoder))
							self.emit("data", self._readableState.decoder.write(buf2));
						else
							self.emit("data", buf2);
					} while (nbytes > 0);
				} else
					_net_.pollResumeWrite(self._socket);
			} else {
				self.close();
			}
		}

		obj.write = function() {
			if (self.destroyed)
				return;
			self._drain = true;
			self.emit("drain");
			if ((!self._buffer) || (!self._buffer.length))
				_net_.pollPauseWrite(self._socket);
			self.flush();
		}
		_net_.poll(this._socket, 1, obj);

		this.write = function(data, encoding, callback) {
			if ((this.destroyed) || (this._socket === undefined) || (!this.tls_context))
				throw new Error("socket is closed");
			if (!_tls_.established(this.tls_context))
				throw new Error("tls connection is not established yet");

			var buf = data;
			if (typeof data === "string") {
				if (!encoding)
					encoding = "utf-8";

				buf = new TextEncoder(encoding).encode(data);
			}
			var sent = 0;
			var tls_write = 0;
			do {
				tls_write = _tls_.write(this.tls_context, buf, buf.length - sent, sent);
				sent += tls_write;
			} while (tls_write > 0);
			return this.flush();
		}

		this.close = function() {
			if (this.tls_context) {
				_tls_.destroy(this.tls_context);
				this.tls_context = undefined;
			}
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

		this.flush = function() {
			var buf = _tls_.getWriteBuffer(this.tls_context);
			if ((buf) && (buf.length === undefined) && (buf.byteLength))
				buf = new Uint8Array(buf);

			if ((buf) && (buf.length)) {
				if (this._buffer) {
					var buf2 = new Uint8Array(this._buffer.length + buf.length);
					buf2.set(this._buffer);
					buf2.set(buf, this._buffer.length);
					this._buffer = buf2;
				} else
					this._buffer = buf;
			}
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

		this.getCipher = function() {
			if (this.tls_context) 
				return { "standardName": _tls_.cipher(this.tls_context) };
		}
	},

	Server: function(options, connectionListener) {
		if (options) {
			var cert = options.cert;
			var key = options.key;
			if (typeof cert != "string")
				cert = new TextDecoder("utf8").decode(cert);
			if (typeof key != "string")
				key = new TextDecoder("utf8").decode(key);
			this.tls_context = _tls_.create(true, cert, key);
		} else
			this.tls_context = _tls_.create(true);
		var self = this;
		_net_server.call(this, function(socket) {
			tls.Socket.call(socket, {"tls_context": _tls_.accept(self.tls_context), "_server": self }, connectionListener);
			socket.on("error", function(err) { socket.close(); });
		});
	},

	createServer(options, connectionListener) {
		if (connectionListener === undefined)
			connectionListener = options;
		return new this.Server(options, connectionListener);
	},

	createConnection(options, connectionListener) {
		return new this.Socket(options, connectionListener);
	},

	connect(options, connectionListener) {
		return createConnection(options, connectionListener);
	}
}

util.inherits(tls.Socket, net.Socket);
util.inherits(tls.Server, net.Server);

module.exports = tls;
