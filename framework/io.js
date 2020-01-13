var EventEmitter = require('events').EventEmitter;
var util = require('util');

function io(fd, read) {
	this._socket = fd;
	var obj = { };
	var self = this;
	if (read) {
		obj.read = function() {
			var buf = new Uint8Array(1024);
			var nbytes = _net_.recv(self._socket, buf, buf.byteLength);
			if (nbytes < 0) {
				self.emit("error", new Error(_net_.strerror(_net_.errno())));
			} else
			if (nbytes) {
				if (nbytes != 1024)
					buf = buf.subarray(0, nbytes);
				if (self.encoding)
					buf = new TextDecoder(self.encoding).decode(buf);
				self.emit("data", buf);
			}
		}

		_net_.poll(this._socket, 0, obj);
	}

	this.write = function(buf) {
		var sent = 0;
		if (typeof buf == "string")
			buf = new TextEncoder(this.encoding ? this.encoding : "utf8").encode(buf);
		while (sent < buf.length) {
			var sent_now = _net_.send(this._socket, buf, buf.length - sent, sent);
			if (sent_now >= 0) {
				if (!sent_now)
					break;
				sent += sent_now;
			} else {
				self.emit("error", new Error(_net_.strerror(_net_.errno())));
				break;
			}
		}
		return sent;
	}

	this.setEncoding = function(encoding) {
		this.encoding = encoding;
		return this;
	}

	this.ref = function() {
		if (!this.__refHandler)
			this.__refHandler = new Array();

		this.__refHandler.push(setInterval(function() { }, 3600000));
		return this;
	}

	this.unref = function() {
		if ((this.__refHandler) && (this.__refHandler.length)) {
			var handler = this.__refHandler.pop();
			if (handler)
				clearInterval(handler);
		}
		return this;
	}

	this.close = function() {
		_net_.unpoll(this._socket);
	}
};

util.inherits(io, EventEmitter);

module.exports = io;
