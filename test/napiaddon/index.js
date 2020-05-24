const { sb_add: bindingSbAdd, set_buffer: setBuffer, add } = require('./build/Release/napi.node');

const buf = Buffer.alloc(12);
setBuffer(buf);

const uint32Arr = new Uint32Array(buf.buffer);

function sbAdd (a, b) {
  uint32Arr[0] = a;
  uint32Arr[1] = b;
  bindingSbAdd();
  return uint32Arr[2];
}

exports.sbAdd = sbAdd;
exports.add = add;
