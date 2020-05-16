# sbffi

A super-quick [FFI](https://en.wikipedia.org/wiki/Foreign_function_interface) for Node.js.

## Usage

**`sbffi.getNativeFunction(pathToSharedLibrary, functionName, returnType, [argType1, argType2, ...])`**

All the arguments are strings. The types must be standard C types. When functions take 64-bit types, the parameters must be passed as BigInts. 64-bit return values will also be BigInts.

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

### Pointers

Pointers are currently assumed to be 64-bit, and can be passed to native functions by specifying the type as `uint64_t`. (Note: this will change in the future!)

You can put raw data into a Buffer, and then get a pointer to the start of that buffer with:

**`const bufferPointer = sbffi.getBufferPointer(buffer);`**

Arrays and strings must be passed as pointers.

## Installation

Installation requires that [`cmake`](https://cmake.org/) is installed. Other than that, this should install as an npm module without issue.
