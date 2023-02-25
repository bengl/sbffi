const binding = require('../build/Release/sbffi.node');
const { sizes } = binding;

const callBuffer = Buffer.alloc(1024);
const callBufferDV = new DataView(callBuffer.buffer, callBuffer.offset);
binding.setCallBuffer(callBuffer);
const callBackBuffer = Buffer.alloc(1024);
const callBackBufferDV = new DataView(callBackBuffer.buffer, callBackBuffer.offset);
binding.setCallBackBuffer(callBackBuffer);

// We'll use this to split BigInt pointers into 32-bit chunks for internal
// pointers (i.e. not ones provided by users).
const splitterBuffer = Buffer.alloc(8);
const splitterBufferDV = new DataView(splitterBuffer.buffer);
function split64 (big) {
  splitterBufferDV.setBigUint64(0, big, true);
  return [
    splitterBufferDV.getUint32(0, true),
    splitterBufferDV.getUint32(4, true)
  ];
}

// These match exactly the fn_type enum in sbffi_common.h
const typeIds = {
  bool: 0,
  char: 1,
  short: 2,
  int: 3,
  long: 4,
  'long long': 5,
  float: 6,
  double: 7,
  pointer: 8,
  void: 9
};

// True sizeof(void) is 1, but that currently breaks return value stuff here, which all assumes
// void's size is zero
sizes.void = 0;

function firstTypeOfSize (n) {
  const bytes = n / 8;
  for (const typ in sizes) {
    if (sizes[typ] === bytes) {
      return typ;
    }
  }
  throw new Error('unknown type size:' + n);
}
const typeAliases = {
  uint8_t: firstTypeOfSize(8),
  uint16_t: firstTypeOfSize(16),
  uint32_t: firstTypeOfSize(32),
  uint64_t: firstTypeOfSize(64),
  int8_t: firstTypeOfSize(8),
  int16_t: firstTypeOfSize(16),
  int32_t: firstTypeOfSize(32),
  int64_t: firstTypeOfSize(64),
  pointer: 'char *'
};

function getTypeId (type) {
  if (type in typeIds) {
    return typeIds[type];
  }
  if (type in typeAliases) {
    return typeIds[typeAliases[type]];
  }
  throw TypeError('invalid type: ' + type);
}

function sizeof (typ) {
  if (typ.includes('*')) return sizes['char *'];
  return sizes[typ];
}

function getWriters (dv) {
  return {
    int8_t: (value, offset) => {
      dv.setInt8(offset, value);
      return offset + 1;
    },
    uint8_t: (value, offset) => {
      dv.setUint8(offset, value);
      return offset + 1;
    },
    int16_t: (value, offset) => {
      dv.setInt16(offset, value, true);
      return offset + 2;
    },
    uint16_t: (value, offset) => {
      dv.setUint16(offset, value, true);
      return offset + 2;
    },
    int32_t: (value, offset) => {
      dv.setInt32(offset, value, true);
      return offset + 4;
    },
    uint32_t: (value, offset) => {
      dv.setUint32(offset, value, true);
      return offset + 4;
    },
    int64_t: (value, offset) => {
      dv.setBigInt64(offset, value, true);
      return offset + 8;
    },
    uint64_t: (value, offset) => {
      dv.setBigUint64(offset, value, true);
      return offset + 8;
    },
    float: (value, offset) => {
      dv.setFloat32(offset, value, true);
      return offset + 4;
    },
    double: (value, offset) => {
      dv.setFloat64(offset, value, true);
      return offset + 8;
    }
  };
}

const writers = getWriters(callBufferDV);
const cbWriters = getWriters(callBackBufferDV);

function getReaders (dv) {
  return {
    int8_t: offset => dv.getInt8(offset),
    uint8_t: offset => dv.getUint8(offset),
    int16_t: offset => dv.getUint16(offset, true),
    uint16_t: offset => dv.getUint16(offset, true),
    int32_t: offset => dv.getUint32(offset, true),
    uint32_t: offset => dv.getUint32(offset, true),
    int64_t: offset => dv.getBigInt64(offset, true),
    uint64_t: offset => dv.getBigUint64(offset, true),
    float: offset => dv.getFloat32(offset, true),
    double: offset => dv.getFloat64(offset, true)
  };
}

const readers = getReaders(callBufferDV);
const cbReaders = getReaders(callBackBufferDV);

// Attempts to map arbitrary C type names to types that have reader/writer functions on buffers.
function normalizeType (typ) {
  if (Array.isArray(typ)) {
    return [
      normalizeType(typ[0]),
      typ[1].map(normalizeType)
    ];
  }
  if (typ in readers) return typ;
  if (typ === 'void') return typ;
  if (typ.includes('*')) return `uint${sizeof('char *') * 8}_t`;
  let unsigned = '';
  if (typ.startsWith('unsigned ')) {
    unsigned = 'u';
    typ = typ.replace('unsigned ', '');
  }
  if (typ in sizes) {
    return `${unsigned}int${sizes[typ] * 8}_t`;
  }
  return typ;
}

function createCbFromUserCb (userCb, [retType, argTypes], oneShot = true) {
  let cbPtr = null;
  const cb = function () {
    try {
      let offset = 0;
      const cbArgs = [];
      for (const typ of argTypes) {
        cbArgs.push(cbReaders[typ](offset));
        offset += sizes[typ];
      }
      const retVal = userCb(...cbArgs);
      if (retType !== 'void') {
        cbWriters[retType](retVal, offset);
      }
    } catch (e) {
      // TODO can we do better than a nextTick here? handle in it C?
      process.nextTick(() => {
        throw e;
      });
    }
    if (oneShot) {
      setImmediate(() => {
        binding.releaseCallback(cbPtr);
      });
    }
  };
  cbPtr = binding.createCallback(getTypeId(retType), argTypes.map(getTypeId), cb);
  return cbPtr;
}

class MultiCallbackPointer {
  constructor (cb, signature) {
    this.cb = createCbFromUserCb(cb, signature, false);
  }

  destroy () {
    binding.releaseCallback(this.cb);
  }
}

function createCallback (cb, [ret, args]) {
  return new MultiCallbackPointer(cb, [normalizeType(ret), args.map(normalizeType)]);
}

const libCache = {};

function getNativeFunction (lib, func, ret, args) {
  ret = normalizeType(ret);
  args = args.map(normalizeType);
  const retBinTyp = getTypeId(ret);
  let libPtr = lib ? libCache[lib] : null;
  if (!libPtr && lib) {
    libPtr = binding.dlLoadLibrary(lib);
    if (!libPtr) {
      throw new Error(`Library not found: ${lib}`);
    }
    libCache[lib] = libPtr;
  }
  const funcPtr = binding.dlFindSymbol(libPtr, func);
  if (!funcPtr) {
    throw new Error(`Function not found: ${func} in library: ${lib}`);
  }
  const callbacks = new Array(args.length);
  let hasCallbacks = false;
  const [callPtr0, callPtr1] = split64(binding.addSignature(
    funcPtr,
    retBinTyp,
    args.map((x, i) => {
      if (Array.isArray(x)) {
        callbacks[i] = true;
        hasCallbacks = true;
        return 'pointer';
      } else {
        return x;
      }
    }).map(getTypeId)
  ));

  const retSize = sizes[ret];
  const argWriters = args.map(x => writers[x]);
  const getReturnVal = readers[ret];

  if (hasCallbacks) {
    return function (...callArgs) {
      let offset = 0;
      callBufferDV.setUint32(offset, callPtr0, true);
      callBufferDV.setUint32(offset + 4, callPtr1, true);
      offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
      for (let i = 0; i < args.length; i++) {
        if (callbacks[i]) {
          const userCb = callArgs[i];
          const cbPtr = (userCb instanceof MultiCallbackPointer)
            ? userCb.cb
            : createCbFromUserCb(userCb, args[i]);
          callBufferDV.setBigUint64(offset, cbPtr, true);
          offset += 8;
        } else {
          offset = argWriters[i](callArgs[i], offset);
        }
      }
      binding.call();
      if (retSize > 0) {
        return getReturnVal(8);
      }
    };
  } else {
    switch (args.length) {
      case 0:
        return function () {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
      case 1:
        return function (arg0) {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          offset = argWriters[0](arg0, offset);
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
      case 2:
        return function (arg0, arg1) {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          offset = argWriters[0](arg0, offset);
          offset = argWriters[1](arg1, offset);
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
      case 3:
        return function (arg0, arg1, arg2) {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          offset = argWriters[0](arg0, offset);
          offset = argWriters[1](arg1, offset);
          offset = argWriters[2](arg2, offset);
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
      case 4:
        return function (arg0, arg1, arg2, arg3) {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          offset = argWriters[0](arg0, offset);
          offset = argWriters[1](arg1, offset);
          offset = argWriters[2](arg2, offset);
          offset = argWriters[3](arg3, offset);
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
      default:
        return function (...callArgs) {
          let offset = 0;
          callBufferDV.setUint32(offset, callPtr0, true);
          callBufferDV.setUint32(offset + 4, callPtr1, true);
          offset += 8 + retSize; // 8 for the callPtr, and more to make space for the retVal.
          for (let i = 0; i < args.length; i++) {
            offset = argWriters[i](callArgs[i], offset);
          }
          binding.call();
          if (retSize > 0) {
            return getReturnVal(8);
          }
        };
    }
  }
}

exports.getNativeFunction = getNativeFunction;
exports.getBufferPointer = binding.getBufPtr;
exports.createCallback = createCallback;
exports.sizeof = sizeof;
