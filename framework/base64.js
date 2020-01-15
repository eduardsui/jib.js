module.exports = {
	base64_encode_data: function(data, len, b64x, b64pad) {
		var dst = "";
		var i;
		function StrAt(data, i) {
			return data.charCodeAt(i);
		}

		function Uint8ArrayAt(data, i) {
			return data[i];
		}

		var data_charCodeAt;
		if (typeof data === "string")
			data_charCodeAt = StrAt;
		else
			data_charCodeAt = Uint8ArrayAt;

		for (i = 0; i <= len - 3; i += 3) {
			dst += b64x[data_charCodeAt(data, i) >>> 2];
			dst += b64x[((data_charCodeAt(data, i) & 3) << 4) | (data_charCodeAt(data, i + 1) >>> 4)];
			dst += b64x[((data_charCodeAt(data, i + 1) & 15) << 2) | (data_charCodeAt(data, i + 2) >>> 6)];
			dst += b64x[data_charCodeAt(data, i + 2) & 63];
		}

		if (len % 3 == 2) {
			dst += b64x[data_charCodeAt(data, i) >>> 2];
			dst += b64x[((data_charCodeAt(data, i) & 3) << 4) | (data_charCodeAt(data, i + 1) >>> 4)];
			dst += b64x[((data_charCodeAt(data, i + 1) & 15) << 2)];
			dst += b64pad;
		} else
		if (len % 3 == 1) {
			dst += b64x[data_charCodeAt(data, i) >>> 2];
			dst += b64x[((data_charCodeAt(data, i) & 3) << 4)];
			dst += b64pad;
			dst += b64pad;
		}
		return dst;
	},

	encode: function(str) {
		return this.base64_encode_data(str, str.length, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", "=");
	},

	urlEncode: function(str) {
		return this.base64_encode_data(str, str.length, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_", "");
	},

	stringify: function(str) {
		return urlEncode(str);
	},

	charIndex: function(c) {
		if (c == "+")
			return 62;
		if (c == "/")
			return 63;
		return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".indexOf(c);
	},

	decode: function(data) {
		var dst = new Uint8Array(data.length);
		var i, a, b, c, d, z;

		while (data.length % 4)
			data += "=";

		var idx = 0;
		for (i = 0; i < data.length - 3; i += 4) {
			a = this.charIndex(data.charAt(i+0));
			b = this.charIndex(data.charAt(i+1));
			c = this.charIndex(data.charAt(i+2));
			d = this.charIndex(data.charAt(i+3));

			dst[idx ++] = (a << 2) | (b >>> 4);
			if (data.charAt(i+2) != "=")
				dst[idx ++] = ((b << 4) & 0xF0) | ((c >>> 2) & 0x0F);
			if (data.charAt(i+3) != "=")
				dst[idx ++] = ((c << 6) & 0xC0) | d;
		}

		return dst.subarray(0, idx);
	},

	urlSniff: function(base64) {
		if (base64.indexOf("-") >= 0)
			return true;
		if (base64.indexOf("_") >= 0)
			return true;
		return false;
	}
}
