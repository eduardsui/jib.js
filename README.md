# jib.js

Jib.js is a lightweight javascript run-time environment, designed to be easily embedded into other applications. It uses mostly standard C functions and some POSIX functions.

Its objectives are:
- small footprint
- easily embeddable
- portable
- no dependecies
- node.js compatiblity
- to be usable as a shell interpreter

It may store all .js modules inside the library/executable (see `src/modules.h`), external I/O being optional.

It currently supports: `fs`, `net`, `http`, `url`, `querystringify`, `events` and Promises.

Compiling
----------

Linux & BSD:
`$ ./build_unix.sh`

Windows:
`build_win32.bat`

macOS:
`$ ./build_macOS.sh`

esp32:
```
$ cd esp32
$ idf.py menuconfig
$ idf.py build
```

Examples
----------

Run a simple web server on localhost on port 8080:
`$ ./jib examples/server.js`

Simple hello world (from node.js website):
```
const http = require('http');

const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) => {
  res.statusCode = 200;
  res.setHeader('Content-Type', 'text/plain');
  res.end('Hello World\n');
});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});
```

It also supports streams:
```
// ...
  var readStream = fs.createReadStream("." + pathname);
  readStream.on('open', function () {
    readStream.pipe(res);
  });

  readStream.on('error', function(err) {
    res.statusCode = 404;
    res.end(err.toString());
  });
// ...
```

License
----------
Where not specified public domain applies.
Some sources from third parties are under BSD, MIT or Apache.
