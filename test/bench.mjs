import './ensure-built.js';

import sbffi from '../lib/index.js';
import ffi from 'ffi-napi';
import { add as napiAdder, sbAdd } from './napiaddon/index.js';
import path from 'path';
import * as url from 'url';
import { performance, createHistogram } from 'perf_hooks';
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

const histFfiAdder = createHistogram();
const perfFfiAdder = performance.timerify(ffiAdder, { histogram: histFfiAdder });

const histSbffiAdder = createHistogram();
const perfSbffiAdder = performance.timerify(sbffiAdder, { histogram: histSbffiAdder });

const histNapiAdder = createHistogram();
const perfNapiAdder = performance.timerify(napiAdder, { histogram: histNapiAdder });

const histSbAdd = createHistogram();
const perfSbAdd = performance.timerify(sbAdd, { histogram: histSbAdd });

const histWasmAdder = createHistogram();
const perfWasmAdder = performance.timerify(wasmAdder, { histogram: histWasmAdder });

const histJsAdder = createHistogram();
const perfJsAdder = performance.timerify(jsAdder, { histogram: histJsAdder });

const ITERATIONS = Number(process.env.ITERATIONS) || 100000;
const REPS = Number(process.env.REPS) || 5;

for (let j = 0; j < REPS; j++) {
  // This one is quite a bit slower, so optionally disable it
  if (!('NO_FFI_NAPI' in process.env)) {
    process.stdout.write('ffi-napi');
    for (let i = 0; i < ITERATIONS; i++) {
      perfFfiAdder(i, i);
    }
    console.log(' ... done!')
  }

  process.stdout.write('sbffi');
  for (let i = 0; i < ITERATIONS; i++) {
    perfSbffiAdder(i, i);
  }
  console.log(' ... done!')

  process.stdout.write('napi-addon');
  for (let i = 0; i < ITERATIONS; i++) {
    perfNapiAdder(i, i);
  }
  console.log(' ... done!')

  process.stdout.write('napi-addon-sb');
  for (let i = 0; i < ITERATIONS; i++) {
    perfSbAdd(i, i);
  }
  console.log(' ... done!')

  process.stdout.write('wasm');
  for (let i = 0; i < ITERATIONS; i++) {
    perfWasmAdder(i, i);
  }
  console.log(' ... done!')

  process.stdout.write('js');
  for (let i = 0; i < ITERATIONS; i++) {
    perfJsAdder(i, i);
  }
  console.log(' ... done!')

  console.log('---');
}

function hist2table(hist) {
  return {
    min: hist.min,
    max: hist.max,
    mean: hist.mean,
    stddev: hist.stddev
  }
}
console.table({
  'ffi-napi': hist2table(histFfiAdder),
  'sbffi': hist2table(histSbffiAdder),
  'napi-addon': hist2table(histNapiAdder),
  'napi-addon-sb': hist2table(histSbAdd),
  'wasm': hist2table(histWasmAdder),
  'js': hist2table(histJsAdder),
})
