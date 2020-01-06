var Path = {
	delimiter: ":",
	sep: "/",

	basename: function(path, ext) {
		var base = path.substring(path.replace(/\\/g, "/").lastIndexOf('/') + 1);
		if ((ext) && (base.lastIndexOf(ext) != -1)) {
			base = base.substring(0, base.lastIndexOf(ext));
		}
		return base;
	},

	dirname: function(path) {
		return path.substring(0, path.replace(/\\/g, "/").lastIndexOf('/'));
	},

	extname: function(path) {
		var idx = Path.basename(path).lastIndexOf('.');
		if (idx < 1)
			return "";
		return Path.basename(path).substring(idx);
	},

	format: function(pathObject) {
		var dir = pathObject.dir ? pathObject.dir : "";
		var root = pathObject.root ? pathObject.root : "";
		var base = pathObject.base ? pathObject.base : "";
		var name = pathObject.name ? pathObject.name : "";
		var ext = pathObject.ext ? pathObject.ext : "";

		var str = "";

		if ((!dir.length) || (!root.length) || (!base.length))
			str = root;

		var add_sep = true;
		if ((root.length) && ((root === dir) || (!dir.length))) {
			add_sep = false;
			if (base.length)
				ext = "";
		}

		if ((add_sep) && (str.length))
			str += Path.sep;
		str += dir;
		if ((add_sep) && (str.length))
			str += Path.sep;
		str += base;
		if ((add_sep) && (str.length))
			str += Path.sep;
		str += name;
		str += ext;

		return Path.normalize(str);
	},

	isAbsolute: function(path) {
		if ((!path) || (!path.length))
			return false;

		if (path[0] == "/")
			return true;

		if (path.length > 1) {
			if (path[0] + path[1] == "\\\\")
				return true;

			if (path[1] == ":") {
				var drive_letter = path[0].toUpperCase();
				if ((path[0] >= 'A') && (path[0] <= 'Z'))
					return true;
			}
		}

		return false;
	},

	join: function() {
		var path = "";
		for (var i = 0; i < arguments.length; i++) {
			path += Path.sep;
			path += arguments[i];
		}
		return Path.normalize(path);
	},

	normalize: function(path) {
		if ((!path) || (!path.length))
			return ".";

		path = path.replace(/\\/g, "/");
		var is_absolute = Path.isAbsolute(path);

		var add_sep = false;
		if ((is_absolute) && ((process.platform !== "win32") || (path.length < 2) || (path[1] !== ":")))
			add_sep = true;

		var arr = path.split("/");

		path = "";
		var skip = false;
		for (var i = arr.length - 1; i >= 0; i --) {
			var dir = arr[i];
			if (!dir.length)
				continue;

			if (dir === ".")
				continue;

			if (dir === "..") {
				skip = true;
				continue;
			}
			if (skip) {
				skip = false;
				continue;
			}
			if (path.length)
				path = dir + Path.sep + path;
			else
				path = dir;
		}
		if (add_sep)
			return Path.sep + path;
		return path;
	},

	parse: function(path) {
		path = Path.normalize(path);
		var root = "";

		if ((path.length > 0) && ((path[0] === "/") || (path[0] === "\\")))
			root = "/";
		else
		if ((path.length > 1) && (path[0] !== "/") && (path[1] === ":"))
			root = path.substring(0, 3);
		var ext = Path.extname(path);
		var base = Path.basename(path);
		var obj = {
			root: root,
			dir: Path.dirname(path),
			base: base,
			ext: ext,
			name: base.substr(0, base.lastIndexOf(ext))
		};
		return obj;
	},

	relative: function(from, to) {
		from = Path.resolve(from);
		to = Path.resolve(to);

		var fromParts = from.split(Path.sep);
		var toParts = to.split(Path.sep);

		var length = Math.min(fromParts.length, toParts.length);
		var samePartsLength = length;
		for (var i = 0; i < length; i++) {
			if (fromParts[i] !== toParts[i]) {
				samePartsLength = i;
				break;
			}
		}
		var outputParts = [];
		for (var i = samePartsLength; i < fromParts.length; i++)
			outputParts.push('..');

		outputParts = outputParts.concat(toParts.slice(samePartsLength));
		return outputParts.join('/');
	},

	resolve: function() {
		var path = "";
		for (var i = 0; i < arguments.length; i++) {
			if (path.length)
				path += Path.sep;
			path += arguments[i];
		}
		path = Path.normalize(path);
		if (Path.isAbsolute(path))
			return path;
		return Path.normalize(process.cwd() + Path.sep + path);
	},

	toNamespacedPath: function(path) {
		return path;
	}
}

if (process.platform === "win32") {
	Path.delimiter = ";";
	Path.sep = "\\";
}

module.exports = Path;
