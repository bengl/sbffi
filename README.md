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

### Callbacks

You can use two different styles of callbacks with `sbffi`.

* **Simple callbacks** are simply passed into the function as normal. They must
  be called _exactly once_ by the underlying native function.

* **Advanced callbacks** must be wrapped with `createCallback`, and `.destroy()`
  must be called on them when the underlying native function will no longer call
  it. There's a slight performance advantage in using advanced callbacks and
  re-using them, since simple callbacks create a `Napi::ThreadsafeFunction` per
  invocation. In addition, APIs that may call the same callback multiple times
  may be used with advanced callbacks, but not with simple callbacks.

In either case, to specify a simple callback, identify it in the arguments array
passed to `getNatveFunction()` as `[cbReturnType, [cbArgTyp1, cbArgType2,
...]]`.

For advanced callbacks, after specifying them in the native function signature,
you can initialize them as follows:

```js
function myCb (result) { /* ... */ }
const advancedCallback = createCallback(myCb, [cbReturnType, [cbArgTyp1, cbArgType2, ...]]);
```

You can then pass `advancedCallback` to a native function that takes in a
callbacks with that signature, just as you would any other callback. When the
callback is no longer needed, you can call `advancedCallback.destroy()`.
_Failure to call `.destroy()` will keep the Node.js process alive._

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
ffi-napi ... done!
sbffi ... done!
napi-addon ... done!
napi-addon-sb ... done!
wasm ... done!
js ... done!
---
ffi-napi ... done!
sbffi ... done!
napi-addon ... done!
napi-addon-sb ... done!
wasm ... done!
js ... done!
---
ffi-napi ... done!
sbffi ... done!
napi-addon ... done!
napi-addon-sb ... done!
wasm ... done!
js ... done!
---
ffi-napi ... done!
sbffi ... done!
napi-addon ... done!
napi-addon-sb ... done!
wasm ... done!
js ... done!
---
ffi-napi ... done!
sbffi ... done!
napi-addon ... done!
napi-addon-sb ... done!
wasm ... done!
js ... done!
---
┌───────────────┬──────┬───────────┬────────────┬────────────────────┐
│    (index)    │ min  │    max    │    mean    │       stddev       │
├───────────────┼──────┼───────────┼────────────┼────────────────────┤
│   ffi-napi    │ 3216 │ 122159103 │ 8271.61785 │ 291769.95704110194 │
│     sbffi     │ 120  │  3657727  │ 177.070514 │ 9480.134200345783  │
│  napi-addon   │ 120  │  2877439  │ 181.689724 │ 9197.256781039703  │
│ napi-addon-sb │  80  │  3256319  │ 143.028966 │ 10193.011672112762 │
│     wasm      │  60  │  3317759  │ 170.443644 │ 16172.981925863021 │
│      js       │  70  │  3395583  │ 141.67375  │ 12967.590912612988 │
└───────────────┴──────┴───────────┴────────────┴────────────────────┘
```

For this benchmark, I generally see roughly similar performance between `sbffi`
and a typical NAPI addon. Of course, YMMV.

## Contributing

Please see [CONTRIBUTING.md](./CONTRIBUTING.md),
[CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and [TODO.md](./TODO.md).

## License

Please see [LICENSE.txt](./LICENSE.txt).
