var crypto = require('crypto');
var base64 = require('base64');

module.exports = {
	sign: function(payload, key, header) {
		if ((!header) || (typeof header != "object"))
			header = {alg: "HS256"};

		var str = base64.urlEncode(JSON.stringify(header)) + "." + base64.urlEncode(JSON.stringify(payload));
		var signature = this.signature(header, key, str);
		if (signature)
			return str + "." + base64.urlEncode(signature);
		return str;
	},

	signature: function(header, key, str) {
		function fromHexString(hexString) {
			return new Uint8Array(hexString.match(/.{1,2}/g).map(function(byte) {return parseInt(byte, 16);}));
		}

		switch (header.alg.toUpperCase()) {
			case "none":
				return "";
			case "HS256":
			default:
				return fromHexString(crypto.HmacSHA256(str, key).toString());
		}
	},

	verifySync: function(token, key) {
		if ((key) && (!key.length))
			key = undefined;
		var arr = token.split(".");
		if (arr.legth < 2)
			return null;

		// no key present
		if ((!key) && (arr.length != 2))
			return null;

		// no signature
		if ((key) && (arr.length != 3))
			return null;

		var decoder = new TextDecoder("utf8");
		var header = JSON.parse(decoder.decode(base64.decode(arr[0])));
		if (!header)
			return null;

		var payload = JSON.parse(decoder.decode(base64.decode(arr[1])));
		if (!payload)
			return null;

		if (!key)
			return payload;

		var sig = this.signature(header, key, arr[0] + "." + arr[1]);
		if (arr[2] === base64.urlEncode(sig)) {
			var now = new Date().getUTCSeconds();
			if (payload.exp) {
				if (payload.exp < now)
					return null;
			}
			if (payload.nbf) {
				if (payload.nbf > now)
					return null;
			}
			return payload;
		}
		return null;
	},

	verify: function(token, key, callback) {
		var obj = this.verifySync(token, key);
		if (!callback)
			return obj;
		var err;
		if (obj === null)
			err = new Error("invalid token");
		setImmediate(function() { callback(err, obj) });
	}	
}
