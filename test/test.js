require('./ensure-built');

const path = require('path');
const assert = require('assert');
const pitesti = require('pitesti');
const { getNativeFunction, getBufferPointer } = require('../lib/index');

const test = pitesti();

const add = {};
const addPtr = {};
const addAsync = {};
const addTwiceAsync = {};
let getpid;

const libAdder = path.join(__dirname, 'adder', 'libadder.so');

const types = [
  { id: 'float', size: 32 },
  { id: 'double', size: 64 }
];
for (const size of [8, 16, 32, 64]) {
  types.push({ id: `int${size}_t`, size: size });
  types.push({ id: `uint${size}_t`, size: size });
}

test`get functions`(() => {
  for (const type of types) {
    const typ = type.id;
    add[typ] = getNativeFunction(
      libAdder,
      `test_add_${typ}`,
      typ,
      [typ, typ]
    );

    addPtr[typ] = getNativeFunction(
      libAdder,
      `test_add_ptr_${typ}`,
      'void',
      [typ + ' *', typ + ' *', typ + ' *']
    );

    addAsync[typ] = getNativeFunction(
      libAdder,
      `test_add_async_${typ}`,
      'void',
      [typ, typ, ['void', [typ]]]
    );

    addTwiceAsync[typ] = getNativeFunction(
      libAdder,
      `test_add_async_twice_${typ}`,
      'void',
      [typ, typ, ['void', [typ]]]
    );

    getpid = getNativeFunction(null, 'getpid', 'int', []);
  }
});

test`non-library function`(() => {
  assert.strictEqual(process.pid, getpid());
});

test`getBufferPointer`(() => {
  const testBuf = Buffer.alloc(10);
  const testBufPtr = getBufferPointer(testBuf);
  assert.strictEqual(typeof testBufPtr, 'bigint');
  assert(testBufPtr > 0n);
});

function num (typ, size, n) {
  return size === 64 && typ !== 'double' ? BigInt(n) : n;
}

function bufAccess (typ, size) {
  if (typ === 'float') return 'FloatLE';
  if (typ === 'double') return 'DoubleLE';
  const u = typ.startsWith('u') ? 'U' : '';
  const big = size === 64 ? 'Big' : '';
  const endian = size === 8 ? '' : 'LE';
  return `${big}${u}Int${size}${endian}`;
}

function read (typ, size, buf, n) {
  return buf[`read${bufAccess(typ, size)}`](n);
}

function write (typ, size, buf, n, i) {
  return buf[`write${bufAccess(typ, size)}`](n, i);
}

for (const type of types) {
  const typ = type.id;
  const size = type.size;
  test.context(typ, () => {
    const n = num.bind(null, typ, size);
    test`basic adding`(() => {
      assert.strictEqual(add[typ](n(0), n(2)), n(2));
      assert.strictEqual(add[typ](n(4), n(5)), n(9));
      assert.strictEqual(add[typ](n(11), n(22)), n(33));
      if (typ.startsWith('u') && size !== 64) {
        assert.strictEqual(add[typ](Math.pow(2, size) - 1, 5), 4);
      }
    });

    const r = read.bind(null, typ, size);
    const w = write.bind(null, typ, size);
    test`adding via pointers`(() => {
      const addingBuf = Buffer.alloc(size * 3);
      w(addingBuf, n(4), 0);
      w(addingBuf, n(3), size / 8);
      const addingBufPtr = getBufferPointer(addingBuf);
      addPtr[typ](addingBufPtr, addingBufPtr + BigInt(size / 8), addingBufPtr + BigInt((size / 8) * 2));
      assert.strictEqual(r(addingBuf, (size / 8) * 2), n(7));
    });

    test`async adding`((done) => {
      addAsync[typ](n(4), n(5), (result) => {
        assert.strictEqual(result, n(9));
        done();
      });
    });

    test`promisified adding`(async () => {
      const addPromise = (a, b) => new Promise(resolve => addAsync[typ](a, b, resolve));
      assert.strictEqual(await addPromise(n(5), n(3)), n(8));
    });

    test`calling callback more than once`((done) => {
      let counter = 0;
      addTwiceAsync[typ](n(4), n(5), (result) => {
        assert.strictEqual(result, n(9));
        if (++counter === 2) {
          done();
        }
      });
    });
  });
}

test();
