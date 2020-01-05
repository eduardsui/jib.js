'use strict';

var EE = require('events').EventEmitter;

module.exports = Stream;

function __unsupportedStream() {
}

util.inherits(__unsupportedStream, EE);

Stream.Readable = __unsupportedStream;
Stream.Writable = __unsupportedStream;
Stream.Duplex = __unsupportedStream;
Stream.Transform = __unsupportedStream;
Stream.PassThrough = __unsupportedStream;

// Backwards-compat with node 0.4.x
Stream.Stream = Stream;

function Stream() {
  EE.call(this);
}

Stream.prototype.pipe = function(dest, options) {
  function onerror(er) {
    cleanup(); 
    if (EE.listenerCount(this, 'error') === 0)
      throw er;
  }

  source.on('error', onerror);
  dest.on('error', onerror); 

  function cleanup() {
    source.removeListener('data', ondata);
    dest.removeListener('drain', ondrain);

    source.removeListener('end', onend);
    source.removeListener('close', onclose);

    source.removeListener('error', onerror);
    dest.removeListener('error', onerror);

    source.removeListener('end', cleanup);
    source.removeListener('close', cleanup);

    dest.removeListener('close', cleanup);
  }

  source.on('end', cleanup);
  source.on('close', cleanup);

  dest.on('close', cleanup);

  return dest;
};

util.inherits(Stream, EE);
