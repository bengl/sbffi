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
[cbArgTyp1, cbArgType2, ...]]`. _Note that currently, callbacks must be called
exactly once. 

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

### Structs

For now, `sbfffi` doesn't have any built-in support for structs. That being
said, there are some helpful libraries like
[`shared-structs`](https://www.npmjs.com/package/shared-structs) and
[`ref-napi`](https://www.npmjs.com/package/ref-napi) (and its family of
modules). As long as you can build up a C struct into a Buffer, you can pass
pointers to them into C functions. Non-pointer struct arguments or return values
are not supported.

## Development

Using a non-release version of `sbffi` requires that
[`cmake`](https://cmake.org/) is installed in order to compile the native
addon.

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
`console.time()`. Here are the results on my machine (AMD Ryzen 9 5900X) with
Ubuntu 22.04.1 and Node.js 19.6.0:

```
ffi-napi: 793.168ms
sbffi: 5.267ms
napi-addon: 3.688ms
napi-addon-sb: 2.298ms
wasm: 1.281ms
js: 1.143ms
---
ffi-napi: 978.664ms
sbffi: 2.883ms
napi-addon: 2.344ms
napi-addon-sb: 0.964ms
wasm: 0.22ms
js: 0.033ms
---
ffi-napi: 1.041s
sbffi: 2.858ms
napi-addon: 2.292ms
napi-addon-sb: 0.98ms
wasm: 0.218ms
js: 0.033ms
---
ffi-napi: 808.294ms
sbffi: 2.878ms
napi-addon: 2.291ms
napi-addon-sb: 0.982ms
wasm: 0.215ms
js: 0.032ms
---
ffi-napi: 1.142s
sbffi: 2.876ms
napi-addon: 2.441ms
napi-addon-sb: 1.161ms
wasm: 0.218ms
js: 0.033ms
---
```

Of course, YMMV.

## Contributing

Please see [CONTRIBUTING.md](./CONTRIBUTING.md),
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [TODO.md](./TODO.md).

## License

Please see [LICENSE.txt](./LICENSE.txt).
