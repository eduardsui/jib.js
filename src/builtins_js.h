#ifndef BUILTINS_JS_H
#define BUILTINS_JS_H

#define JS_PROMISE  \
    "function Promise(fn) {\n" \
      "const PENDING = 0;\n" \
      "const FULFILLED = 1;\n" \
      "const REJECTED = 2;\n" \
      "var state = PENDING;\n" \
      "var value = null;\n" \
      "var handlers = [];\n" \
      "function fulfill(result) {\n" \
        "state = FULFILLED;\n" \
        "value = result;\n" \
        "handlers.forEach(handle);\n" \
        "handlers = null;\n" \
      "}\n" \
      "function reject(error) {\n" \
        "state = REJECTED;\n" \
        "value = error;\n" \
        "handlers.forEach(handle);\n" \
        "handlers = null;\n" \
      "}\n" \
      "function resolve(result) {\n" \
        "try {\n" \
          "var then = getThen(result);\n" \
          "if (then) {\n" \
            "doResolve(then.bind(result), resolve, reject)\n" \
            "return\n" \
          "}\n" \
          "fulfill(result);\n" \
        "} catch (e) {\n" \
          "reject(e);\n" \
        "}\n" \
      "}\n" \
      "function handle(handler) {\n" \
        "if (state === PENDING) {\n" \
          "handlers.push(handler);\n" \
        "} else {\n" \
          "if (state === FULFILLED &&\n" \
            "typeof handler.onFulfilled === 'function') {\n" \
            "handler.onFulfilled(value);\n" \
          "}\n" \
          "if (state === REJECTED &&\n" \
            "typeof handler.onRejected === 'function') {\n" \
            "handler.onRejected(value);\n" \
          "}\n" \
        "}\n" \
      "}\n" \
      "this.done = function (onFulfilled, onRejected) {\n" \
        "setTimeout(function () {\n" \
          "handle({\n" \
            "onFulfilled: onFulfilled,\n" \
            "onRejected: onRejected\n" \
          "});\n" \
        "}, 0);\n" \
      "}\n" \
      "this.then = function (onFulfilled, onRejected) {\n" \
        "var self = this;\n" \
        "return new Promise(function (resolve, reject) {\n" \
          "return self.done(function (result) {\n" \
            "if (typeof onFulfilled === 'function') {\n" \
              "try {\n" \
                "return resolve(onFulfilled(result));\n" \
              "} catch (ex) {\n" \
                "return reject(ex);\n" \
              "}\n" \
            "} else {\n" \
              "return resolve(result);\n" \
            "}\n" \
          "}, function (error) {\n" \
            "if (typeof onRejected === 'function') {\n" \
              "try {\n" \
                "return resolve(onRejected(error));\n" \
              "} catch (ex) {\n" \
                "return reject(ex);\n" \
              "}\n" \
            "} else {\n" \
              "return reject(error);\n" \
            "}\n" \
          "});\n" \
        "});\n" \
      "}\n" \
      "function getThen(value) {\n" \
        "var t = typeof value;\n" \
        "if (value && (t === 'object' || t === 'function')) {\n" \
          "var then = value.then;\n" \
          "if (typeof then === 'function') {\n" \
            "return then;\n" \
          "}\n" \
        "}\n" \
        "return null;\n" \
      "}\n" \
      "function doResolve(fn, onFulfilled, onRejected) {\n" \
        "var done = false;\n" \
        "try {\n" \
          "fn(function (value) {\n" \
            "if (done) return\n" \
            "done = true\n" \
            "onFulfilled(value)\n" \
          "}, function (reason) {\n" \
            "if (done) return\n" \
            "done = true\n" \
            "onRejected(reason)\n" \
          "})\n" \
        "} catch (ex) {\n" \
          "if (done) return\n" \
          "done = true\n" \
          "onRejected(ex)\n" \
        "}\n" \
      "}\n" \
      "doResolve(fn, resolve, reject);\n" \
    "}\n"

#define JS_GLOBAL \
    "if (typeof global === 'undefined') {" \
        "(function () {" \
            "var global = new Function('return this;')();" \
            "Object.defineProperty(global, 'global', {" \
                "value: global," \
                "writable: true," \
                "enumerable: false," \
                "configurable: true" \
            "});" \
        "})();" \
    "}\n"

#define JS_MODULE \
    "function Module() {\n" \
       "var id;\n" \
       "var filename;\n" \
       "var parent;\n" \
       "var loaded = false;\n" \
       "var children = [ ];\n" \
       "var exports;\n" \
    "}"

#define JS_WINDOW \
    "function HTMLUIWindow() {\n" \
       "this.data = function(name) { var data = this.call(name ? name : 'data'); if (typeof data === 'string') return JSON.parse(data); return data; };\n" \
    "}\n"

#define JS_STATS \
    "function Stats() {\n" \
       "this.dev = undefined;\n" \
       "this.ino = undefined;\n" \
       "this.mode = undefined;\n" \
       "this.nlink = undefined;\n" \
       "this.uid = undefined;\n" \
       "this.gid = undefined;\n" \
       "this.rdev = undefined;\n" \
       "this.size = undefined;\n" \
       "this.blksize = undefined;\n" \
       "this.block = undefined;\n" \
       "this.atime = undefined;\n" \
       "this.mtime = undefined;\n" \
       "this.ctime = undefined;\n" \
       "this.isBlockDevice = function() { return ((this.mode & 0xF000) === 0x6000); };\n" \
       "this.isCharacterDevice = function() { return ((this.mode & 0xF000) === 0x2000); };\n" \
       "this.isDirectory = function() { return ((this.mode & 0xF000) === 0x4000); };\n" \
       "this.isFIFO = function() { return ((this.mode & 0xF000) === 0x1000); };\n" \
       "this.isFile = function() { return ((this.mode & 0xF000) === 0x8000); };\n" \
       "this.isSocket = function() { return ((this.mode & 0xF000) === 0xC000); };\n" \
       "this.isSymbolicLink = function() { return ((this.mode & 0xF000) === 0xA000); };\n" \
    "}\n"

#define JS_CONSOLE \
    "console.stringify = function(obj, max_level, level) {\n" \
        "if (obj === undefined)\n" \
            "return 'undefined';\n" \
        "if (!max_level)\n" \
            "max_level = 2;\n" \
        "var prefix = '';\n" \
        "if (!level)\n" \
            "level = 1;\n" \
        "var prefix_2 = '';\n" \
        "for (var i = 0; i < level; i ++) {\n" \
            "prefix += '  ';\n" \
            "if (i < level - 1)\n" \
                "prefix_2 += '  ';\n" \
        "}\n" \
        "var str = '';\n" \
        "str += '[object ' + obj.constructor.name + '] {';\n" \
        "var max_len = 0;\n" \
        "for (var k in obj) {\n" \
            "if (k.length > max_len)\n" \
                "max_len = k.length;\n" \
        "}\n" \
        "var obj_str = '';\n" \
        "for (var k in obj) {\n" \
            "obj_str += prefix;\n" \
            "var str_k = k;\n" \
            "while (str_k.length < max_len)\n" \
                "str_k += ' ';\n" \
            "obj_str += str_k;\n" \
            "obj_str += ': ';\n" \
            "var obj_k = obj[k];\n" \
            "if ((level <= max_level) && (typeof obj_k === 'object') && (obj_k !== undefined) && (obj_k !== null) && (!(obj_k instanceof Array)) && (obj_k !== obj))\n" \
                "obj_str += this.stringify(obj_k, max_level, level + 1);\n" \
            "else\n" \
            "if (obj_k === null)\n" \
                "obj_str += 'null';\n" \
            "else\n" \
            "if (obj_k === undefined)\n" \
                "obj_str += 'undefined';\n" \
            "else\n" \
                "obj_str += obj_k.toString();\n" \
            "obj_str += '\\n';\n" \
        "}\n" \
        "if (obj_str.length == 0)\n" \
            "str += ' }';\n" \
        "else\n" \
            "str += '\\n' + obj_str + prefix_2 + '}';\n" \
        "return str;\n" \
    "}\n"

#define JS_GPIO_DUMMY \
    "var gpio = {" \
        "padSelectGpio: function() { console.warn('padSelectGpio not implemented'); }," \
        "setDirection: function() { console.warn('setDirection not implemented'); }," \
        "setLevel: function() { console.warn('setLevel not implemented'); }," \
        "getLevel: function() { console.warn('getLevel not implemented'); }," \
        "setIntrType: function() { console.warn('setIntrType not implemented'); }," \
        "constants: { GPIO_MODE_INPUT: 0, GPIO_MODE_OUTPUT: 1, GPIO_MODE_AF: 2, GPIO_MODE_ANALOG: 3, GPIO_INTR_DISABLE: 0, GPIO_INTR_POSEDGE: 1, GPIO_INTR_NEGEDGE: 2, GPIO_INTR_ANYEDGE: 3, GPIO_INTR_LOW_LEVEL: 4, GPIO_INTR_HIGH_LEVEL: 5 }" \
    "}"

#endif
