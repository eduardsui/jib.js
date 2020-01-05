'use strict';

var isArray = module.exports.isArray = Array.isArray;

function isBoolean(arg) {
  return typeof arg === 'boolean';
}
module.exports.isBoolean = isBoolean;

function isNull(arg) {
  return arg === null;
}
module.exports.isNull = isNull;

function isNullOrUndefined(arg) {
  return arg == null;
}
module.exports.isNullOrUndefined = isNullOrUndefined;

function isNumber(arg) {
  return typeof arg === 'number';
}
module.exports.isNumber = isNumber;

function isString(arg) {
  return typeof arg === 'string';
}
module.exports.isString = isString;

function isSymbol(arg) {
  return typeof arg === 'symbol';
}
module.exports.isSymbol = isSymbol;

function isUndefined(arg) {
  return arg === void 0;
}
module.exports.isUndefined = isUndefined;

function isRegExp(re) {
  return isObject(re) && objectToString(re) === '[object RegExp]';
}
module.exports.isRegExp = isRegExp;

function isObject(arg) {
  return typeof arg === 'object' && arg !== null;
}
module.exports.isObject = isObject;

function isDate(d) {
  return isObject(d) && objectToString(d) === '[object Date]';
}
module.exports.isDate = isDate;

function isError(e) {
  return isObject(e) &&
      (objectToString(e) === '[object Error]' || e instanceof Error);
}
module.exports.isError = isError;

function isFunction(arg) {
  return typeof arg === 'function';
}
module.exports.isFunction = isFunction;

function isPrimitive(arg) {
  return arg === null ||
         typeof arg === 'boolean' ||
         typeof arg === 'number' ||
         typeof arg === 'string' ||
         typeof arg === 'symbol' ||  // ES6 symbol
         typeof arg === 'undefined';
}
module.exports.isPrimitive = isPrimitive;

function isBuffer(arg) {
  return arg instanceof Buffer;
}
module.exports.isBuffer = isBuffer;

module.exports.inherits = function(ctor, superCtor) {
  ctor.super_ = superCtor;
  ctor.prototype = Object.create(superCtor.prototype, {
    constructor: {
      value: ctor,
      enumerable: false,
      writable: true,
      configurable: true
    }
  });
};

module.exports._extend = function(origin, add) {
  // Don't do anything if add isn't an object
  if (!add || !isObject(add)) return origin;

  var keys = Object.keys(add);
  var i = keys.length;
  while (i--) {
    origin[keys[i]] = add[keys[i]];
  }
  return origin;
};

function hasOwnProperty(obj, prop) {
  return Object.prototype.hasOwnProperty.call(obj, prop);
}
