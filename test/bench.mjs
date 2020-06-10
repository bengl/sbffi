import './ensure-built.js';

import sbffi from '../lib/index.js';
import ffi from 'ffi-napi';
import { add as napiAdder, sbAdd } from './napiaddon/index.js';
import path from 'path';
import * as url from 'url';
const __filename = url.fileURLToPath(import.meta.url);
const __dirname = url.fileURLToPath(new URL('.', import.meta.url));

const libraryPath = path.join(__dirname, 'adder', 'libadder.so');

let u32 = 'ulong';
const { test_add_uint32_t: ffiAdder } = ffi.Library(libraryPath, {
  test_add_uint32_t: [u32, [u32, u32]]
});

u32 = 'uint32_t';
const sbffiAdder = sbffi.getNativeFunction(libraryPath, 'test_add_uint32_t', u32, [u32, u32]);

const { add: wasmAdder } = await import('./adder/adder.wasm');

function jsAdder (a, b) {
  return a + b;
}

const ITERATIONS = Number(process.env.ITERATIONS) || 100000;
const REPS = Number(process.env.REPS) || 5;


for (let j = 0; j < REPS; j++) {
  // This one is quite a bit slower, so optionally disable it
  if (!('NO_FFI_NAPI' in process.env)) {
    console.time('ffi-napi');
    for (let i = 0; i < ITERATIONS; i++) {
      ffiAdder(i, i);
    }
    console.timeEnd('ffi-napi');
  }

  sbffi.setCallSpeed('slow');
  console.time('sbffi');
  for (let i = 0; i < ITERATIONS; i++) {
    sbffiAdder(i, i);
  }
  console.timeEnd('sbffi');

  sbffi.setCallSpeed('fast');
  console.time('sbffi-fastcall');
  for (let i = 0; i < ITERATIONS; i++) {
    sbffiAdder(i, i);
  }
  console.timeEnd('sbffi-fastcall');

  console.time('napi-addon');
  for (let i = 0; i < ITERATIONS; i++) {
    napiAdder(i, i);
  }
  console.timeEnd('napi-addon');

  console.time('napi-addon-sb');
  for (let i = 0; i < ITERATIONS; i++) {
    sbAdd(i, i);
  }
  console.timeEnd('napi-addon-sb');

  console.time('wasm');
  for (let i = 0; i < ITERATIONS; i++) {
    wasmAdder(i, i);
  }
  console.timeEnd('wasm');

  console.time('js');
  for (let i = 0; i < ITERATIONS; i++) {
    jsAdder(i, i);
  }
  console.timeEnd('js');

  console.log('---');
}
