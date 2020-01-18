var jiblib = {
	makeKey: function(seed) {
		if (!seed)
			seed = process.randomBytes(32);

		seed[0] &= 248;
		seed[31] &= 127;
		seed[31] |= 64;

		var mypublic = new Uint8Array(32);
		_crypto_.x25519(mypublic, seed);

		var base32_encode = require("base32").encode;
		return { "public": base32_encode(mypublic), "private": base32_encode(seed) };
	},

	jibprotocol: function(network_key, keyId, privateKey, frequency, tx_interval, max_relay) {
		var base32_decode = undefined;

		if (!keyId) {
			var keyPair = jiblib.makeKey();
			keyId = keyPair.public;
			privateKey = keyPair.private;
		}

		this.txInterval =  tx_interval ? tx_interval :  10000;
		if ((keyId) && (typeof keyId === "string")) {
			console.log("Device id is " + keyId);
			base32_decode = require("base32").decode;
			this.keyId = new Uint8Array(base32_decode(keyId));
		} else {
			this.keyId = keyId;
		}
		if ((privateKey) && (typeof privateKey === "string")) {
			if (!base32_decode)
				base32_decode = require("base32").decode;
			this.privateKey = new Uint8Array(base32_decode(privateKey));
		} else {
			this.privateKey = privateKey;
		}
		if (this.keyId)
			this.keyIdCrc = _crypto_.crc16(this.keyId);
		else
			this.keyIdCrc = 0;
		this._friends = { };
		this.netKey = network_key;
		this._sent_cache = { };
		this.maxRelay = max_relay ? max_relay : 100;

		lora.init();
		lora.setFrequency(frequency ? frequency : 433e6);
		lora.setTxPower(17);
		lora.setSpreadingFactor(12);
		// lora.setBandwidth(20.8E3);
		lora.setCodingRate(8);
	
		lora.reset();
		lora.receive();

		this.encoder = undefined;
		this.encoding = "utf8";

		this.queue_repeater = [ ];
		this.queue = [ ];

		this.nextIO = Date.now();

		var self = this;

		var md5 = function(d){if(typeof d!="string"){d.charCodeAt=function(i){return d[i];}}result = M(V(Y(X(d),8*d.length)));return result.toLowerCase()};function M(d){for(var _,m="0123456789ABCDEF",f="",r=0;r<d.length;r++)_=d.charCodeAt(r),f+=m.charAt(_>>>4&15)+m.charAt(15&_);return f}function X(d){for(var _=Array(d.length>>2),m=0;m<_.length;m++)_[m]=0;for(m=0;m<8*d.length;m+=8)_[m>>5]|=(255&d.charCodeAt(m/8))<<m%32;return _}function V(d){for(var _="",m=0;m<32*d.length;m+=8)_+=String.fromCharCode(d[m>>5]>>>m%32&255);return _}function Y(d,_){d[_>>5]|=128<<_%32,d[14+(_+64>>>9<<4)]=_;for(var m=1732584193,f=-271733879,r=-1732584194,i=271733878,n=0;n<d.length;n+=16){var h=m,t=f,g=r,e=i;f=md5_ii(f=md5_ii(f=md5_ii(f=md5_ii(f=md5_hh(f=md5_hh(f=md5_hh(f=md5_hh(f=md5_gg(f=md5_gg(f=md5_gg(f=md5_gg(f=md5_ff(f=md5_ff(f=md5_ff(f=md5_ff(f,r=md5_ff(r,i=md5_ff(i,m=md5_ff(m,f,r,i,d[n+0],7,-680876936),f,r,d[n+1],12,-389564586),m,f,d[n+2],17,606105819),i,m,d[n+3],22,-1044525330),r=md5_ff(r,i=md5_ff(i,m=md5_ff(m,f,r,i,d[n+4],7,-176418897),f,r,d[n+5],12,1200080426),m,f,d[n+6],17,-1473231341),i,m,d[n+7],22,-45705983),r=md5_ff(r,i=md5_ff(i,m=md5_ff(m,f,r,i,d[n+8],7,1770035416),f,r,d[n+9],12,-1958414417),m,f,d[n+10],17,-42063),i,m,d[n+11],22,-1990404162),r=md5_ff(r,i=md5_ff(i,m=md5_ff(m,f,r,i,d[n+12],7,1804603682),f,r,d[n+13],12,-40341101),m,f,d[n+14],17,-1502002290),i,m,d[n+15],22,1236535329),r=md5_gg(r,i=md5_gg(i,m=md5_gg(m,f,r,i,d[n+1],5,-165796510),f,r,d[n+6],9,-1069501632),m,f,d[n+11],14,643717713),i,m,d[n+0],20,-373897302),r=md5_gg(r,i=md5_gg(i,m=md5_gg(m,f,r,i,d[n+5],5,-701558691),f,r,d[n+10],9,38016083),m,f,d[n+15],14,-660478335),i,m,d[n+4],20,-405537848),r=md5_gg(r,i=md5_gg(i,m=md5_gg(m,f,r,i,d[n+9],5,568446438),f,r,d[n+14],9,-1019803690),m,f,d[n+3],14,-187363961),i,m,d[n+8],20,1163531501),r=md5_gg(r,i=md5_gg(i,m=md5_gg(m,f,r,i,d[n+13],5,-1444681467),f,r,d[n+2],9,-51403784),m,f,d[n+7],14,1735328473),i,m,d[n+12],20,-1926607734),r=md5_hh(r,i=md5_hh(i,m=md5_hh(m,f,r,i,d[n+5],4,-378558),f,r,d[n+8],11,-2022574463),m,f,d[n+11],16,1839030562),i,m,d[n+14],23,-35309556),r=md5_hh(r,i=md5_hh(i,m=md5_hh(m,f,r,i,d[n+1],4,-1530992060),f,r,d[n+4],11,1272893353),m,f,d[n+7],16,-155497632),i,m,d[n+10],23,-1094730640),r=md5_hh(r,i=md5_hh(i,m=md5_hh(m,f,r,i,d[n+13],4,681279174),f,r,d[n+0],11,-358537222),m,f,d[n+3],16,-722521979),i,m,d[n+6],23,76029189),r=md5_hh(r,i=md5_hh(i,m=md5_hh(m,f,r,i,d[n+9],4,-640364487),f,r,d[n+12],11,-421815835),m,f,d[n+15],16,530742520),i,m,d[n+2],23,-995338651),r=md5_ii(r,i=md5_ii(i,m=md5_ii(m,f,r,i,d[n+0],6,-198630844),f,r,d[n+7],10,1126891415),m,f,d[n+14],15,-1416354905),i,m,d[n+5],21,-57434055),r=md5_ii(r,i=md5_ii(i,m=md5_ii(m,f,r,i,d[n+12],6,1700485571),f,r,d[n+3],10,-1894986606),m,f,d[n+10],15,-1051523),i,m,d[n+1],21,-2054922799),r=md5_ii(r,i=md5_ii(i,m=md5_ii(m,f,r,i,d[n+8],6,1873313359),f,r,d[n+15],10,-30611744),m,f,d[n+6],15,-1560198380),i,m,d[n+13],21,1309151649),r=md5_ii(r,i=md5_ii(i,m=md5_ii(m,f,r,i,d[n+4],6,-145523070),f,r,d[n+11],10,-1120210379),m,f,d[n+2],15,718787259),i,m,d[n+9],21,-343485551),m=safe_add(m,h),f=safe_add(f,t),r=safe_add(r,g),i=safe_add(i,e)}return Array(m,f,r,i)}function md5_cmn(d,_,m,f,r,i){return safe_add(bit_rol(safe_add(safe_add(_,d),safe_add(f,i)),r),m)}function md5_ff(d,_,m,f,r,i,n){return md5_cmn(_&m|~_&f,d,_,r,i,n)}function md5_gg(d,_,m,f,r,i,n){return md5_cmn(_&f|m&~f,d,_,r,i,n)}function md5_hh(d,_,m,f,r,i,n){return md5_cmn(_^m^f,d,_,r,i,n)}function md5_ii(d,_,m,f,r,i,n){return md5_cmn(m^(_|~f),d,_,r,i,n)}function safe_add(d,_){var m=(65535&d)+(65535&_);return(d>>16)+(_>>16)+(m>>16)<<16|65535&m}function bit_rol(d,_){return d<<_|d>>>32-_}

		this.id = function() {
			if (this.keyId)
				return require("base32").encode(this.keyId);
			return "";
		}

		this.addFriend = function(name, key) {
			if (typeof key === "string") {
				if (!base32_decode)
					base32_decode = require("base32").decode;
				key = new Uint8Array(base32_decode(key));
			}
			if (!key)
				return this;

			this._friends[name] = key;
			return this;
		}

		function pack(data, msg_type, destination_key) {
			if (data.length > 44) {
				console.warn("message to big");
				return undefined;
			}
			var index = 4;
			if (destination_key)
				index += 2;

			var header = new Uint8Array(index + 1 + data.length);
			if (!msg_type)
				msg_type = 0x00;
		
			header[index] = msg_type;

			for (var i = 0; i < data.length; i ++)
				header[i + index + 1] = data[i];

			var crc = _crypto_.crc16(header.subarray(index));

			header[0] = Math.floor(crc / 0x100);
			header[1] = crc % 0x100;

			var dest_keyId = 0;
			if (typeof destination_key === "string") {
				if (!base32_decode)
					base32_decode = require("base32").decode;
				destination_key = new Uint8Array(base32_decode(destination_key));
			}
			if (destination_key)
				dest_keyId = _crypto_.crc16(destination_key);

			header[2] = Math.floor(dest_keyId / 0x100);
			header[3] = dest_keyId % 0x100;

			var payload = _crypto_.chacha20(header.subarray(index), self.netKey);

			if (!payload) {
				console.warn("error encrypting message");
				return undefined;
			}

			payload = header.subarray(index);
			if (destination_key) {
				var shared = new Uint8Array(32);
				_crypto_.x25519(shared, self.privateKey, destination_key);
				payload = _crypto_.chacha20(payload, shared);

				crc = _crypto_.crc16(payload);

				header[4] = Math.floor(crc / 0x100);
				header[5] = crc % 0x100;
			}

			var payload = _crypto_.chacha20(payload, self.netKey);
			if (!payload) {
				console.warn("error encrypting message");
				return undefined;
			}

			for (var i = 0; i < payload.length; i ++)
				header[i + index] = payload[i];

			return header;
		}

		this.send = function(data, destination_key, msg_type) {
			if (typeof data === "string") {
				if (!this.encoder)
					this.encoder = new TextEncoder(this.encoding);
				data = this.encoder.encode(data);
			}
			if ((destination_key) && (typeof destination_key === "string") && (this._friends[destination_key]))
				destination_key = this._friends[destination_key];

			if (!msg_type)
				msg_type = 0;
			var buf = pack(data, msg_type, destination_key);
			if (buf) {
				this.queue.push(buf);
				console.log("queued message (" + this.queue.length + " messages)");
			}
			return this;
		}

		function unpack(buf) {
			if ((!buf) || (buf.length < 5)) {
				console.warn("buffer too small");
				return undefined;
			}

			var crc = buf[0] * 0x100 + buf[1];
			var key_id = buf[2] * 0x100 + buf[3];
			var index = 4;
			var crc2 = 0;
			if (key_id) {
				if (buf.length < 7) {
					console.warn("buffer too small");
					return undefined;
				}
				crc2 = buf[4] * 0x100 + buf[5];
				index = 6;
			}


			var payload = _crypto_.chacha20(buf.subarray(index), self.netKey);
			if (!payload) {
				console.warn("error decrypting message");
				return undefined;
			}

			if (key_id) {
				if (_crypto_.crc16(payload) != crc2) {
					console.warn("dropping invalid message (invalid crc)", crc2, _crypto_.crc16(payload));
					return undefined;
				}

				if (key_id !== self.keyIdCrc) {
					console.log("message is not for this device");
					return { "keyId": key_id, "relay": true };
				}
				
				var shared = new Uint8Array(32);
				for (k in self._friends) {
					var key = self._friends[k];
					_crypto_.x25519(shared, self.privateKey, key);
					var payload2 = _crypto_.chacha20(payload, shared);
					if (_crypto_.crc16(payload2) == crc)
						return { "message": payload2.subarray(1), "keyId": key_id, "type": payload2[0], "from": k };
				}
				console.warn("dropping invalid message (no matching key)");
				return { "keyId": key_id, "relay": true };
			}

			if (_crypto_.crc16(payload) != crc) {
				console.warn("dropping invalid message");
				return undefined;
			}
			return { "message": payload.subarray(1), "keyId": key_id, "type": payload[0] };
		}

		function clean(limit_ms) {
			var limit = Date.now();
			for (var k in self._sent_cache) {
				if (self._sent_cache[k] + limit_ms <= limit)
					delete self._sent_cache[k];
			}
		}

		this.start = function(timeout) {
			timeout = timeout ? timeout : 200;
			lora.receive();
			this.send_interval = setInterval(function() {
				if (lora.received()) {
					buf = lora.recv();
					lora.receive();
					if (buf) {
						var msg = unpack(buf);
						if (msg) {
							var k = md5(buf);
							if (!self._sent_cache[k]) {
								self._sent_cache[k] = Date.now();
								console.log("received message");
								if (((!msg.keyId) || (msg.keyId == self.keyIdCrc)) && (self.onmessage) && (!msg.relay))
									self.onmessage(msg);

								if ((!msg.keyId) || (msg.keyId != self.keyIdCrc) || (msg.relay)) {
									if (self.queue_repeater.length >= self.maxRelay) {
										self.queue_repeater.pop();
										console.warn("dequeued repeater (too many messages)");
									}
									self.queue_repeater.push(buf);
									console.log("queued repeating message (" + self.queue_repeater.length + " messages)");
								}
							} else {
								console.warn("received echo message");
							}
						}
					}
					lora.receive();
					clean(120000);
				}
				if ((self.queue.length) || (self.queue_repeater.length)) {
					if (Date.now() >= self.nextIO) {
						var buf;
						if (self.queue.length) {
							buf = self.queue.pop();
							console.log("dequeue own message");
						} else {
							buf = self.queue_repeater.pop();
							console.log("dequeue repeater message");
						}
						lora.send(buf);
						self.nextIO = Date.now() + self.txInterval;
						lora.receive();

						self._sent_cache[md5(buf)] = Date.now();
					}
				}
				lora.receive();
			}, timeout);
		}
	}
}

module.exports = jiblib;
