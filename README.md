# sbffi

A super-quick [FFI](https://en.wikipedia.org/wiki/Foreign_function_interface)
for Node.js.

[`dyncall`](https://dyncall.org/) is used to make dynamic calls to native
functions. In order to avoid some cost of translating JavaScript values into raw
C types, a shared buffer is used for both arguments and return values. Writing
values to a buffer turns out to be quite a bit faster than unpacking them in
native code.

## Usage

**`sbffi.getNativeFunction(pathToSharedLibrary, functionName, returnType, [argType1, argType2, ...])`**

All the arguments are strings. The types must be standard C types. When
functions take 64-bit types, the parameters must be passed as BigInts. 64-bit
return values will also be BigInts.

```
// some C library compiled to libadder.so

uint32_t add(uint32_t a, uint32_t b) {
  return a + b;
}
```

```
// index.js

const { getNativeFunction } = require('sbffi');

const libPath = '/path/to/libadder.so';
const add = getNativeFunction(libPath, 'add', 'uint32_t', ['uint32_t', 'uint32_t']);

const result = add(23, 34);
// 57
```

To specify a callback, identify it in the arguments array as `[cbReturnType,
[cbArgTyp1, cbArgType2, ...]]`.

### Pointers

Pointers are currently assumed to be 64-bit, and can be passed to native
functions by specifying the type as `pointer` or referring to any other type
with an asterisk in the string, for example: `uint8_t *`.

You can put raw data into a Buffer, and then get a pointer to the start of that
buffer with:

**`const bufferPointer = sbffi.getBufferPointer(buffer);`**

Arrays and strings must be passed as pointers.

## Installation

Installation requires that [`cmake`](https://cmake.org/) is installed. Other
than that, this should install as an npm module without issue.

## Benchmarks

A simple benchmark can be run with `npm run bench`. This will test calling a
simple adding function from the test library using the following techniques:

* **`ffi-napi`**: A successor to `node-ffi` compatible with modern versions of
  Node.js.
* **`napi-addon`**: A very simple/normal Node.js addon using NAPI in C.
* **`sbffi`**: This library.
* **`js`**: Re-implementing the function in plain JavaScript.

Each function will be called 100000 times, in 5 repetitions, timed with
`console.time()`. Here are the results on my machine (2019 Lenovo X1 Extreme,
running Ubuntu):

```
ffi-napi: 920.891ms
sbffi: 44.159ms
napi-addon: 7.942ms
js: 2.847ms
ffi-napi: 772.344ms
sbffi: 33.896ms
napi-addon: 4.274ms
js: 0.06ms
ffi-napi: 754.909ms
sbffi: 33.842ms
napi-addon: 4.166ms
js: 0.065ms
ffi-napi: 759.859ms
sbffi: 34.272ms
napi-addon: 4.02ms
js: 0.061ms
ffi-napi: 763.716ms
sbffi: 34.181ms
napi-addon: 4.103ms
js: 0.061ms
```

Of course, YMMV.
