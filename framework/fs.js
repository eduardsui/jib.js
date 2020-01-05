var Stream = require('stream').Stream;
var util = require('util');
var EventEmitter = require('events').EventEmitter;

var kMinPoolSpace = 128;

var fs = {
	F_OK: 0,
	R_OK: 4,
	W_OK: 2,
	X_OK: 1,

	constants: {
		UV_FS_SYMLINK_DIR: 1,
		UV_FS_SYMLINK_JUNCTION: 2,
		O_RDONLY: 0,
		O_WRONLY: 1,
		O_RDWR: 2,
		UV_DIRENT_UNKNOWN: 0,
		UV_DIRENT_FILE: 1,
		UV_DIRENT_DIR: 2,
		UV_DIRENT_LINK: 3,
		UV_DIRENT_FIFO: 4,
		UV_DIRENT_SOCKET: 5,
		UV_DIRENT_CHAR: 6,
		UV_DIRENT_BLOCK: 7,
		S_IFMT: 61440,
		S_IFREG: 32768,
		S_IFDIR: 16384,
		S_IFCHR: 8192,
		S_IFLNK: 40960,
		O_CREAT: 256,
		O_EXCL: 1024,
		UV_FS_O_FILEMAP: 536870912,
		O_TRUNC: 512,
		O_APPEND: 8,
		F_OK: 0,
		R_OK: 4,
		W_OK: 2,
		X_OK: 1,
		UV_FS_COPYFILE_EXCL: 1,
		COPYFILE_EXCL: 1,
		UV_FS_COPYFILE_FICLONE: 2,
		COPYFILE_FICLONE: 2,
		UV_FS_COPYFILE_FICLONE_FORCE: 4,
		COPYFILE_FICLONE_FORCE: 4,
	},

	appendFile: function(path, data, options, callback) {
		if (!callback) {
			callback = options;
			options = undefined;
		}
		var self = this;
		setImmediate(function() { var err = self.appendFileSync(path, data, options); callback(err != 0 ? new Error(_C_library.strerror(err)) : undefined); });
	},

	appendFileSync: function (path, data, options) {
		var append_options = { "flag": "a" };
		if ((options !== undefined) && (options !== null)) {
			switch (typeof options) {
				case "string":
					append_options.encoding = options;
					break;
				case "object":
					if (options.mode)
						append_options.mode = options.mode;
					if (options.flag)
						append_options.flag = options.flag;
					break;
			}
		}
		return this.writeFileSync(path, data, append_options);
	},

	access: function(path, mode, callback) {
		var self = this;
		if (!callback) {
			callback = mode;
			mode = undefined;
		}
		setImmediate(function() { callback(self.accessSync(path, mode) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	accessSync: function(path, mode) {
		if (mode === undefined)
			mode = fs.constants.F_OK;
		return _C_library.access(path, mode);
	},

	chown: function(path, uid, gid, callback) {
		var self = this;
		setImmediate(function() { callback(self.chownSync(path, uid, gid) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	chownSync: function(path, uid, gid) {
		return _C_library.chown(path, uid, gid);
	},

	chmod: function(path, mode, callback) {
		var self = this;
		setImmediate(function() { callback(self.chmodSync(path, mode) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	chmodSync: function(path, mode) {
		return _C_library.chmod(path, mode);
	},

	close: function(fd, callback) {
		var self = this;
		setImmediate(function() { callback(self.closeSync(fd) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	closeSync: function(fd) {
		delete this._finalizers["fd" + fd];
		return _C_library.close(fd);
	},

	exists: function(path, callback) {
		var self = this;
		setImmediate(function() { callback(self.existsSync(path)); });
	},

	existsSync: function(path) {
		var stat = _C_library.stat(path);
		if (stat)
			return true;
		return false;
	},

	fchown: function(fd, uid, gid, callback) {
		var self = this;
		setImmediate(function() { callback(self.fchownSync(fd, uid, gid) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	fchownSync: function(fd, uid, gid) {
		return _C_library.fchown(fd, uid, gid);
	},


	fchmod: function(fd, mode, callback) {
		var self = this;
		setImmediate(function() { callback(self.fchmodSync(fd, mode) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	fchmodSync: function(fd, mode) {
		return _C_library.chmod(fd, mode);
	},

	fdatasync: function(fd) {
		var self = this;
		setImmediate(function() { callback(self.fdatasyncSync(fd) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	fdatasyncSync: function(fd) {
		return _C_library.fdatasync(fd);
	},

	fstat: function(fd, callback) {
		var self = this;
		setImmediate(function() { var stat = self.fstatSync(fd); callback(stat ? undefined : new Error(_C_library.strerror(_C_library.errno())), stat); });
	},

	fstatSync: function(fd) {
		return _C_library.fstat(fd);
	},

	fsync: function(fd, callback) {
		var self = this;
		setImmediate(function() { callback(self.fsyncSync(fd) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	fsyncSync: function(fd) {
		return _C_library.fsync(fd);
	},

	ftruncate: function(fd, len, callback) {
		var self = this;
		if (!callback) {
			callback = mode;
			len = 0;
		}
		setImmediate(function() { callback(self.ftruncateSync(fd, len) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	ftruncateSync: function(path, len) {
		if (!len)
			len = 0;
		return _C_library.truncate(fd, len);
	},

	lchown: function(path, uid, gid, callback) {
		var self = this;
		setImmediate(function() { callback(self.lchownSync(path, uid, gid) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	lchownSync: function(path, uid, gid) {
		return _C_library.lchown(path, uid, gid);
	},

	lchmod: function(path, mode, callback) {
		var self = this;
		setImmediate(function() { callback(self.lchmodSync(path, mode) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	lchmodSync: function(path, mode) {
		return _C_library.lchmod(path, mode);
	},

	link: function(existingPath, newPath, callback) {
		var self = this;
		setImmediate(function() { callback(self.linkSync(existingPath, newPath) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	linkSync: function(existingPath, newPath) {
		return _C_library.link(existingPath, newPath);
	},

	lstat: function(path, callback) {
		var self = this;
		setImmediate(function() { var stat = self.lstatSync(path); callback(stat ? undefined : new Error(_C_library.strerror(_C_library.errno())), stat); });
	},

	lstatSync: function(path) {
		return _C_library.lstat(path);
	},

	mkdir: function(path, mode, callback) {
		var self = this;
		if (!callback) {
			callback = mode;
			mode = undefined;
		}
		setImmediate(function() { callback(self.mkdirSync(path, mode) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	mkdirSync: function(path, mode) {
		if (mode === undefined)
			mode = 0o777;
		return _C_library.mkdir(path, mode);
	},

	open: function(path, flags, mode, callback) {
		var self = this;
		if (callback === undefined) {
			callback = mode;
			mode = 0o666;
			if (callback === undefined) {
				callback = flags;
				flags = 'r';
			}
		}
		setImmediate(function() { var fd = self.openSync(path, flags, mode); callback((fd < 0) ? new Error(_C_library.strerror(_C_library.errno())) : undefined, fd); });
	},

	_parseFlags: function(flags) {
		var real_flags;
		switch (flags) {
			case 'a':
			case 'as':
				real_flags = this.constants.O_WRONLY | this.constants.O_APPEND | this.constants.O_CREAT;
				break;
			case 'ax':
				real_flags = this.constants.O_WRONLY | this.constants.O_APPEND | this.constants.O_EXCL;
				break;
			case 'a+':
			case 'as+':
				real_flags = this.constants.O_RDWR | this.constants.O_APPEND;
				break;
			case 'ax+':
				real_flags = this.constants.O_RDWR | this.constants.O_APPEND  | this.constants.O_EXCL;
				break;
			case 'r':
				real_flags = this.constants.O_RDONLY;
				break;
			case 'rs+':
				real_flags = this.constants.O_RDWR;
				break;
			case 'w':
				real_flags = this.constants.O_WRONLY | this.constants.O_CREAT | this.constants.O_TRUNC;
				break;
			case 'wx':
				real_flags = this.constants.O_WRONLY | this.constants.O_TRUNC;
				break;
			case 'w+':
				real_flags = this.constants.O_RDWR | this.constants.O_CREAT | this.constants.O_TRUNC;
				break;
			case 'wx+':
				real_flags = this.constants.O_RDWR | this.constants.O_CREAT | this.constants.O_TRUNC | this.constants.O_EXCL;
				break;
		}
		return real_flags;
	},

	openSync: function(path, flags, mode) {
		if (mode === undefined)
			mode = 0o666;
		if (flags === undefined)
			flags = 'r';
		return _C_library.open(path, this._parseFlags(flags), mode);
	},

	Dirent: function(path, name) {
		this.name = name;
		this.path = path;

		var stat = _C_library.stat(path + "/" + name);
		if (stat) {
			this.isBlockDevice = stat.isBlockDevice;
			this.isCharacterDevice= stat.isCharacterDevice;
			this.isDirectory = stat.isDirectory;
			this.isFIFO = stat.isFIFO;
			this.isFile = stat.isFile;
			this.isSocket = stat.isSocket;
			this.isSymbolicLink = stat.isSymbolicLink;
		}
	},

	Dir: function(ptr, path, options, Dirent) {
		if ((!options) || (typeof options !== "object"))
			options = { "encoding": "utf-8", "bufferSize": 32 };

		this.options = options;
		this.ptr = ptr;
		this.path = path;

		this.read = function(callback) {
			var self = this;
			setImmediate(function() { var item = self.readSync(); callback(item ? undefined : new Error(_C_library.strerror(_C_library.errno())), item); });
		}

		this.readSync = function() {
			if (this.ptr === undefined)
				return null;
			var item = _C_library.readdir(this.ptr);
			var dirent = null;
			if (item)
				dirent = new Dirent(item, this.path);
			return dirent;
		}

		this.close = function(callback) {
			var self = this;
			setImmediate(function() { callback(self.closeSync() ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
		}

		this.closeSync = function() {
			if (this.ptr !== undefined) {
				var err = _C_library.closedir(this.ptr);
				this.ptr = undefined;
				return err;
			}
			return 0;
		}

		this.isOpened = function() {
			return (this.ptr !== undefined);
		}

		this.finalize = function(self) {
			self.closeSync();
		}

		global.__destructor(this, this.finalize);
	},

	opendir: function(path, options, callback) {
		var self = this;
		if (!callback) {
			callback = options;
			options = undefined;
		}
		setImmediate(function() { var dir = self.opendirSync(path, options); callback(dir ? undefined: new Error(_C_library.strerror(_C_library.errno())), dir); });
	},

	opendirSync: function(path, options) {
		var ptr = _C_library.opendir(path);
		if (ptr !== undefined)
			return new this.Dir(ptr, path, options, this.Dirent);
		return undefined;
	},

	readdir: function(path, options, callback) {
		var self = this;
		if (!callback) {
			callback = options;
			options = undefined;
		}
		setImmediate(function() { var items = self.readdirSync(path, options); callback(items ? undefined: new Error(_C_library.strerror(_C_library.errno())), items); });
	},

	readdirSync: function(path, options) {
		var dir = this.opendirSync(path, options);
		if (!dir)
			return undefined;
		
		var max_reads = dir.options.bufferSize;
		var files = [ ];
		for (var i = 0; i < max_reads; i ++) {
			var ent = _C_library.readdir(dir.ptr);
			if (!ent)
				break;
			files.push(ent);
		}
		dir.closeSync();

		return files;
	},

	read: function(fd, buffer, offset, length, position, callback) {
		var self = this;
		setImmediate(function() { var bytesRead = self.readSync(fd, buffer, offset, length, position); callback((bytesRead < 0) ? new Error(_C_library.strerror(_C_library.errno())) : undefined, bytesRead, buffer); });
	},

	readSync: function(fd, buffer, offset, length, position) {
		if ((position !== null) && (position !== undefined)) {
			if (_C_library.seek(fd, position) < 0)
				return -1;
		}
		return _C_library.read(fd, buffer, length, offset);
	},

	readFile: function(path, options, callback) {
		if (!callback) {
			callback = options;
			options = undefined;
		}
		var self = this;
		setImmediate(function() { var buffer = self.readFileSync(path, options); callback(buffer ? undefined : new Error(_C_library.strerror(_C_library.errno())), buffer); });
	},

	readFileSync: function(path, options) {
		var fd = -1;
		var close_fd = false;
		switch (typeof path) {
			case "string":
				var flag = this.constants.O_RDONLY;
				if ((options) && (typeof options == "object") && (options.flag))
					flag = this._parseFlags(options.flag);
				fd = _C_library.open(path, flag);
				if (fd < 0)
					return -1;
				close_fd = 1;
				break;
			default:
				fd = path;
				break;
		}
		var stat = _C_library.fstat(fd);
		if (!stat)
			return -1;
		var to_read = stat.size;
		var buffer = new ArrayBuffer(to_read);
		var offset = 0;
		var err = false;
		while (to_read > 0) {
			var bytesRead = _C_library.read(fd, buffer, to_read, offset);
			if (bytesRead <= 0) {
				if (bytesRead < 0)
					err = true;
				break;
			}
			offset += bytesRead;
			to_read -= bytesRead;
		}
		if (close_fd)
			_C_library.close(fd);
		if (err)
			return undefined;
		if (options) {
			var encoding;
			if (typeof options == "string")
				encoding = options;
			else
				encoding = options.encoding;
			if (encoding)
				return new TextDecoder(encoding).decode(buffer);
		}
		return buffer;
	},

	realpath: function(path, options, callback) {		
		var self = this;
		setImmediate(function() { var real_path = self.realPathSync(path, options); callback(real_path ? undefined : new Error(_C_library.strerror(_C_library.errno())), real_path); });
	},

	realpathSync: function(path, options) {
		return _C_library.realpath(path);
	},

	rename: function(oldPath, newPath, callback) {
		var self = this;
		setImmediate(function() { callback(self.renameSync(oldPath, newPath) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	renameSync: function(oldPath, newPath) {
		return _C_library.rename(oldPath, newPath);
	},

	rmdir: function(path, callback) {
		var self = this;
		setImmediate(function() { callback(self.rmdirSync(path) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	rmdirSync: function(path) {
		return _C_library.rmdir(path);
	},

	stat: function(path, callback) {
		var self = this;
		setImmediate(function() { var stat = self.statSync(path); callback(stat ? undefined : new Error(_C_library.strerror(_C_library.errno())), stat); });
	},

	statSync: function(path) {
		return _C_library.stat(path);
	},

	symlink: function(target, linkpath, type, callback) {
		if (!callback) {
			callback = type;
			type = undefined;
		}
		var self = this;
		setImmediate(function() { callback(self.symlinkSync(target, linkpath, type) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	symlinkSync: function(target, linkpath, type) {
		return _C_library.symlink(target, linkpath);
	},

	truncate: function(path, len, callback) {
		var self = this;
		if (!callback) {
			callback = mode;
			len = 0;
		}
		setImmediate(function() { callback(self.truncateSync(path, len) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	truncateSync: function(path, len) {
		if (!len)
			len = 0;
		return _C_library.truncate(path, len);
	},

	unlink: function(path) {
		var self = this;
		setImmediate(function() { callback(self.unlinkSync(path) ? new Error(_C_library.strerror(_C_library.errno())) : undefined); });
	},

	unlinkSync: function() {
		return _C_library.unlink(path);
	},

	writeFile: function(file, data, options, callback) {
		if (!callback) {
			callback = options;
			options = undefined;
		}
		var self = this;
		setImmediate(function() { var err = self.WriteFileSync(path, data, options); callback(err != 0 ? new Error(_C_library.strerror(err)) : undefined); });
	},

	writeFileSync: function(file, data, options) {
		var fd = -1;
		var close_fd = false;
		switch (typeof file) {
			case "string":
				var flag = this.constants.O_WRONLY | this.constants.O_CREAT | this.constants.O_TRUNC;
				var mode = 0o666;
				if ((options) && (typeof options == "object")) {
					if (options.flag)
						flag = this._parseFlags(options.flag);
					if (options.mode)
						mode = options.mode;
				}
				fd = _C_library.open(file, flag, mode);
				if (fd < 0)
					return _C_library.errno();
				close_fd = 1;
				break;
			default:
				fd = file;
				break;
		}
		var buffer = data;
		if (typeof buffer === "string") {
			var encoding = "utf-8";
			if (options) {
				if (typeof options == "string")
					encoding = options;
				else
					encoding = options.encoding;
				if (!encoding)
					encoding = "utf-8";
			}
			buffer = (new TextEncoder(encoding).encode(buffer));
		}
			
		var offset = 0;
		to_write = buffer.length;
		var err = 0;
		while (to_write > 0) {
			var bytesWritten = _C_library.write(fd, buffer, to_write, offset);
			if (bytesWritten <= 0) {
				err = _C_library.errno();
				break;
			}
			offset += bytesWritten;
			to_write -= bytesWritten;
		}			
		if (close_fd)
			_C_library.close(fd);
		return err;
	},

	write: function(fd, buffer, offset, length, position, callback) {
		var self = this;
		if (!callback) {
			callback = position;
			position = undefined;
			if (!callback) {
				callback = length;
				length = undefined;
				if (!callback) {
					callback = offset;
					offset = undefined;
				}
			}
		}
		setImmediate(function() { var bytesWritten = self.writeSync(fd, buffer, offset, length, position); callback((bytesWritten < 0) ? new Error(_C_library.strerror(_C_library.errno())) : undefined, bytesWritten, buffer); });
	},

	writeSync: function(fd, buffer, offset, length, position) {
		if ((position !== null) && (position !== undefined)) {
			if (_C_library.seek(fd, position) < 0)
				return -1;
		}
		if ((offset === undefined) || (offset === null))
			offset = 0;
		if ((length === undefined) || (length === null))
			length = buffer.length;
		return _C_library.write(fd, buffer, length, offset);
	},

	copyFile: undefined,
	copyFileSync: undefined,
	mkdtemp: undefined,
	mkdtempSync: undefined,
	readlink: undefined,
	readlinkSync: undefined,
	unwatchFile: undefined,
	utimes: undefined,
	utimesSync: undefined,
	watch: undefined,
	watchFile: undefined,

	_finalizers: { },

	finalize: function(self) {
		for (k in self._finalizers) {
			var fd = finalizers[k];
			if (fd >= 0)
				_C_library.close(fd);
		}
	}
}

module.exports = fs;

var pool;

function allocNewPool(poolSize) {
  pool = new Buffer(poolSize);
  pool.used = 0;
}



fs.createReadStream = function(path, options) {
  return new ReadStream(path, options);
};

util.inherits(ReadStream, Stream.Readable);
fs.ReadStream = ReadStream;

function ReadStream(path, options) {
  if (!(this instanceof ReadStream))
    return new ReadStream(path, options);

  // a little bit bigger buffer and water marks by default
  options = util._extend({
    highWaterMark: 64 * 1024
  }, options || {});

  Stream.Readable.call(this, options);

  this.path = path;
  this.fd = options.hasOwnProperty('fd') ? options.fd : null;
  this.flags = options.hasOwnProperty('flags') ? options.flags : 'r';
  this.mode = options.hasOwnProperty('mode') ? options.mode : 438; /*=0666*/

  this.start = options.hasOwnProperty('start') ? options.start : undefined;
  this.end = options.hasOwnProperty('end') ? options.end : undefined;
  this.autoClose = options.hasOwnProperty('autoClose') ?
      options.autoClose : true;
  this.pos = undefined;

  if (!util.isUndefined(this.start)) {
    if (!util.isNumber(this.start)) {
      throw TypeError('start must be a Number');
    }
    if (util.isUndefined(this.end)) {
      this.end = Infinity;
    } else if (!util.isNumber(this.end)) {
      throw TypeError('end must be a Number');
    }

    if (this.start > this.end) {
      throw new Error('start must be <= end');
    }

    this.pos = this.start;
  }

  if (!util.isNumber(this.fd))
    this.open();

  this.on('end', function() {
    if (this.autoClose) {
      this.destroy();
    }
  });
}

fs.FileReadStream = fs.ReadStream; // support the legacy name

ReadStream.prototype.open = function() {
  var self = this;
  fs.open(this.path, this.flags, this.mode, function(er, fd) {
    if (er) {
      if (self.autoClose) {
        self.destroy();
      }
      self.emit('error', er);
      return;
    }

    self.fd = fd;
    self.emit('open', fd);
    // start the flow of data.
    self.read();
  });
};

ReadStream.prototype._read = function(n) {
  if (!util.isNumber(this.fd))
    return this.once('open', function() {
      this._read(n);
    });

  if (this.destroyed)
    return;

  if (!pool || pool.length - pool.used < kMinPoolSpace) {
    // discard the old pool.
    pool = null;
    allocNewPool(this._readableState.highWaterMark);
  }

  // Grab another reference to the pool in the case that while we're
  // in the thread pool another read() finishes up the pool, and
  // allocates a new one.
  var thisPool = pool;
  var toRead = Math.min(pool.length - pool.used, n);
  var start = pool.used;

  if (!util.isUndefined(this.pos))
    toRead = Math.min(this.end - this.pos + 1, toRead);

  // already read everything we were supposed to read!
  // treat as EOF.
  if (toRead <= 0)
    return this.push(null);

  // the actual read.
  var self = this;
  fs.read(this.fd, pool, pool.used, toRead, this.pos, onread);

  // move the pool positions, and internal position for reading.
  if (!util.isUndefined(this.pos))
    this.pos += toRead;
  pool.used += toRead;

  function onread(er, bytesRead) {
    if (er) {
      if (self.autoClose) {
        self.destroy();
      }
      self.emit('error', er);
    } else {
      var b = null;
      if (bytesRead > 0)
        b = thisPool.slice(start, start + bytesRead);
      self.push(b);
    }
  }
};


ReadStream.prototype.destroy = function() {
  if (this.destroyed)
    return;
  this.destroyed = true;

  if (util.isNumber(this.fd))
    this.close();
};


ReadStream.prototype.close = function(cb) {
  var self = this;
  if (cb)
    this.once('close', cb);
  if (this.closed || !util.isNumber(this.fd)) {
    if (!util.isNumber(this.fd)) {
      this.once('open', close);
      return;
    }
    return process.nextTick(this.emit.bind(this, 'close'));
  }
  this.closed = true;
  close();

  function close(fd) {
    fs.close(fd || self.fd, function(er) {
      if (er)
        self.emit('error', er);
      else
        self.emit('close');
    });
    self.fd = null;
  }
};




fs.createWriteStream = function(path, options) {
  return new WriteStream(path, options);
};

util.inherits(WriteStream, Stream.Writable);
fs.WriteStream = WriteStream;
function WriteStream(path, options) {
  if (!(this instanceof WriteStream))
    return new WriteStream(path, options);

  options = options || {};

  Stream.Writable.call(this, options);

  this.path = path;
  this.fd = null;

  this.fd = options.hasOwnProperty('fd') ? options.fd : null;
  this.flags = options.hasOwnProperty('flags') ? options.flags : 'w';
  this.mode = options.hasOwnProperty('mode') ? options.mode : 438; /*=0666*/

  this.start = options.hasOwnProperty('start') ? options.start : undefined;
  this.pos = undefined;
  this.bytesWritten = 0;

  if (!util.isUndefined(this.start)) {
    if (!util.isNumber(this.start)) {
      throw TypeError('start must be a Number');
    }
    if (this.start < 0) {
      throw new Error('start must be >= zero');
    }

    this.pos = this.start;
  }

  if (!util.isNumber(this.fd))
    this.open();

  // dispose on finish.
  this.once('finish', this.close);
}

fs.FileWriteStream = fs.WriteStream; // support the legacy name


WriteStream.prototype.open = function() {
  fs.open(this.path, this.flags, this.mode, function(er, fd) {
    if (er) {
      this.destroy();
      this.emit('error', er);
      return;
    }

    this.fd = fd;
    this.emit('open', fd);
  }.bind(this));
};


WriteStream.prototype._write = function(data, encoding, cb) {
  if (!util.isBuffer(data))
    return this.emit('error', new Error('Invalid data'));

  if (!util.isNumber(this.fd))
    return this.once('open', function() {
      this._write(data, encoding, cb);
    });

  var self = this;
  fs.write(this.fd, data, 0, data.length, this.pos, function(er, bytes) {
    if (er) {
      self.destroy();
      return cb(er);
    }
    self.bytesWritten += bytes;
    cb();
  });

  if (!util.isUndefined(this.pos))
    this.pos += data.length;
};


WriteStream.prototype.destroy = ReadStream.prototype.destroy;
WriteStream.prototype.close = ReadStream.prototype.close;

// There is no shutdown() for files.
WriteStream.prototype.destroySoon = WriteStream.prototype.end;


// SyncWriteStream is internal. DO NOT USE.
// Temporary hack for process.stdout and process.stderr when piped to files.
function SyncWriteStream(fd, options) {
  Stream.call(this);

  options = options || {};

  this.fd = fd;
  this.writable = true;
  this.readable = false;
  this.autoClose = options.hasOwnProperty('autoClose') ?
      options.autoClose : true;
}

util.inherits(SyncWriteStream, Stream);


// Export
fs.SyncWriteStream = SyncWriteStream;


SyncWriteStream.prototype.write = function(data, arg1, arg2) {
  var encoding, cb;

  // parse arguments
  if (arg1) {
    if (util.isString(arg1)) {
      encoding = arg1;
      cb = arg2;
    } else if (util.isFunction(arg1)) {
      cb = arg1;
    } else {
      throw new Error('bad arg');
    }
  }
  assertEncoding(encoding);

  // Change strings to buffers. SLOW
  if (util.isString(data)) {
    data = new Buffer(data, encoding);
  }

  fs.writeSync(this.fd, data, 0, data.length);

  if (cb) {
    process.nextTick(cb);
  }

  return true;
};


SyncWriteStream.prototype.end = function(data, arg1, arg2) {
  if (data) {
    this.write(data, arg1, arg2);
  }
  this.destroy();
};


SyncWriteStream.prototype.destroy = function() {
  if (this.autoClose)
    fs.closeSync(this.fd);
  this.fd = null;
  this.emit('close');
  return true;
};

SyncWriteStream.prototype.destroySoon = SyncWriteStream.prototype.destroy; 

global.__destructor(module.exports, module.exports.finalize);
