const path = require('path');
const assert = require('assert');
const pitesti = require('pitesti');
const { getNativeFunction, getBufferPointer } = require('../lib/index');

const test = pitesti();

let add;
let addPtr;
let addAsync;

test`get functions`(() => {
  add = getNativeFunction(
    path.join(__dirname, 'adder', 'libadder.so'),
    'test_add_uint32_t',
    'uint32_t',
    ['uint32_t', 'uint32_t']
  );

  addPtr = getNativeFunction(
    path.join(__dirname, 'adder', 'libadder.so'),
    'test_add_ptr_uint32_t',
    'void',
    ['uint32_t *', 'uint32_t *', 'uint32_t *']
  );

  addAsync = getNativeFunction(
    path.join(__dirname, 'adder', 'libadder.so'),
    'test_add_async_uint32_t',
    'void',
    ['uint32_t', 'uint32_t', ['void', ['uint32_t']]]
  );
});

test`basic adding`(() => {
  assert.strictEqual(add(0, 2), 2);
  assert.strictEqual(add(4, 5), 9);
  assert.strictEqual(add(11, 22), 33);
  assert.strictEqual(add(Math.pow(2, 32) - 1, 5), 4);
});

test`getBufferPointer`(() => {
  const testBuf = Buffer.alloc(10);
  const testBufPtr = getBufferPointer(testBuf);
  assert.strictEqual(typeof testBufPtr, 'bigint')
  assert(testBufPtr > 0n);
});

test`adding via pointers`(() => {
  const addingBuf = Buffer.alloc(12);
  addingBuf.writeUInt32LE(4);
  addingBuf.writeUInt32LE(3, 4);
  const addingBufPtr = getBufferPointer(addingBuf);
  addPtr(addingBufPtr, addingBufPtr + 4n, addingBufPtr + 8n);
  assert.strictEqual(addingBuf.readUInt32LE(8), 7);
})

test`async adding`((done) => {
  addAsync(4, 5, (result) => {
    assert.strictEqual(result, 9);
    done();
  });
});

test`promisified adding`(async () => {
  const addPromise = (a, b) => new Promise(resolve => addAsync(a, b, resolve));
  assert.strictEqual(await addPromise(5, 3), 8);
});

test();
