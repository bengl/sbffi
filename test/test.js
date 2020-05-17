const assert = require('assert');
const { getNativeFunction, getBufferPointer } = require('../lib/index');
const path = require('path');

const add = getNativeFunction(
  path.join(__dirname, 'adder', 'libadder.so'),
  'test_add_uint32_t',
  'uint32_t',
  ['uint32_t', 'uint32_t']
);

assert.strictEqual(add(0, 2), 2);
assert.strictEqual(add(4, 5), 9);
assert.strictEqual(add(11, 22), 33);
assert.strictEqual(add(Math.pow(2, 32) - 1, 5), 4);

const testBuf = Buffer.alloc(10);
const testBufPtr = getBufferPointer(testBuf);
assert.strictEqual(typeof testBufPtr, 'bigint')
assert(testBufPtr > 0n);

const addPtr = getNativeFunction(
  path.join(__dirname, 'adder', 'libadder.so'),
  'test_add_ptr_uint32_t',
  'void',
  ['uint32_t *', 'uint32_t *', 'uint32_t *']
);

const addingBuf = Buffer.alloc(12);
addingBuf.writeUInt32LE(4);
addingBuf.writeUInt32LE(3, 4);
const addingBufPtr = getBufferPointer(addingBuf);
addPtr(addingBufPtr, addingBufPtr + 4n, addingBufPtr + 8n);
assert.strictEqual(addingBuf.readUInt32LE(8), 7);


const addAsync = getNativeFunction(
  path.join(__dirname, 'adder', 'libadder.so'),
  'test_add_async_uint32_t',
  'void',
  ['uint32_t', 'uint32_t', ['void', ['uint32_t']]]
);

addAsync(4, 5, (result) => {
  assert.strictEqual(result, 9);
  process.exit(0);
});
