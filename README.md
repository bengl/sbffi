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

All the arguments are strings. The types must be standard C types. See the
**Types** section below for details. When functions take 64-bit types, the
parameters must be passed as BigInts. 64-bit return values will also be
BigInts.

```c
// adder.c: some C library compiled to libadder.so

uint32_t add(uint32_t a, uint32_t b) {
  return a + b;
}
```

```js
// index.js

const { getNativeFunction } = require('sbffi');

const libPath = '/path/to/libadder.so';
const add = getNativeFunction(libPath, 'add', 'uint32_t', ['uint32_t', 'uint32_t']);

const result = add(23, 34);
// 57
```

To specify a callback, identify it in the arguments array as `[cbReturnType,
[cbArgTyp1, cbArgType2, ...]]`.

### Types

The following types are supported:

* `(u)int[8|16|32|64]_t`
* `bool`
* `(unsigned) char`
* `(unsigned) short`
* `(unsigned) int`
* `(unsigned) long`
* `(unsigned) long long`
* `float`
* `double`
* `size_t`

128-bit types are not yet supported, and while this list may grow over time, for
now other types can be used if they're aliases of the above types.

See the section below about pointers.

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
* **`sbffi`**: This library.
* **`napi-addon`**: A very simple/normal Node.js addon using NAPI in C.
* **`napi-addon-sb`**: A NAPI addon using the same shared-buffer technique as
  `sbffi`, but with a hard-coded function call, rather than a dynamic/FFI call.
* **`wasm`**: The adding function compiled to WebAssembly.
* **`js`**: Re-implementing the function in plain JavaScript.

Each function will be called 100000 times, in 5 repetitions, timed with
`console.time()`. Here are the results on my machine (2019 Lenovo X1 Extreme,
running Ubuntu, Node v12):

```
ffi-napi: 1103.680ms
sbffi: 39.981ms
napi-addon: 8.214ms
napi-addon-sb: 6.795ms
wasm: 2.802ms
js: 2.644ms
---
ffi-napi: 1128.388ms
sbffi: 97.446ms
napi-addon: 3.631ms
napi-addon-sb: 3.308ms
wasm: 0.918ms
js: 0.045ms
---
ffi-napi: 1419.159ms
sbffi: 29.797ms
napi-addon: 3.946ms
napi-addon-sb: 3.717ms
wasm: 0.871ms
js: 0.090ms
---
ffi-napi: 1285.210ms
sbffi: 73.335ms
napi-addon: 4.618ms
napi-addon-sb: 3.651ms
wasm: 0.930ms
js: 0.096ms
---
ffi-napi: 772.013ms
sbffi: 29.467ms
napi-addon: 3.790ms
napi-addon-sb: 3.352ms
wasm: 0.847ms
js: 0.087ms
---
```

Of course, YMMV.

## Contributing

Please see [CONTRIBUTING.md](./CONTRIBUTING.md),
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [TODO.md](./TODO.md).

## License

Please see [LICENSE.txt](./LICENSE.txt).
