const binding = require('../build/Release/sbffi.node');
const { sizes: typSizes } = binding;

const callBuffer = Buffer.alloc(1024);
binding.setCallBuffer(callBuffer);
const callBackBuffer = Buffer.alloc(1024);
binding.setCallBackBuffer(callBackBuffer);

/* eslint-disable camelcase */
const [
  fn_type_bool,
  fn_type_char,
  fn_type_short,
  fn_type_int,
  fn_type_long,
  fn_type_long_long,
  fn_type_float,
  fn_type_double,
  fn_type_pointer,
  fn_type_void
] = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
/* eslint-enable camelcase */

const typeIds = {
  bool: fn_type_bool,
  char: fn_type_char,
  short: fn_type_short,
  int: fn_type_int,
  long: fn_type_long,
  'long long': fn_type_long_long,
  float: fn_type_float,
  double: fn_type_double,
  pointer: fn_type_pointer,
  void: fn_type_void
};

function firstTypeOfSize (n) {
  const bytes = n / 8;
  for (const typ in typSizes) {
    if (typSizes[typ] === bytes) {
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
  pointer: firstTypeOfSize(typSizes['char *']),
  size_t: firstTypeOfSize(typSizes.size_t)
};

function mapBinType (type) {
  if (type in typeIds) {
    return typeIds[type];
  }
  if (type in typeAliases) {
    return typeIds[typeAliases[type]];
  }
  throw TypeError('invalid type: ' + type);
}

const sizes = Object.assign({}, typSizes, {
  uint8_t: 1,
  uint16_t: 2,
  uint32_t: 4,
  uint64_t: 8,
  int8_t: 1,
  int16_t: 2,
  int32_t: 4,
  int64_t: 8,
  float: 4,
  double: 8,
  void: 0,
  pointer: typSizes['char *']
});

function sizeof (typ) {
  if (typ.includes('*')) return sizeof('pointer');
  return sizes[typ];
}

const rwTypeMap = {
  pointer: `uint${typSizes['char *'] * 8}_t`
};

function mapRWType (typ) {
  if (typ in writers) return typ;
  if (typ in rwTypeMap) return rwTypeMap[typ];
}

const writers = {
  int8_t: callBuffer.writeInt8.bind(callBuffer),
  uint8_t: callBuffer.writeUInt8.bind(callBuffer),
  int16_t: callBuffer.writeInt16LE.bind(callBuffer),
  uint16_t: callBuffer.writeUInt16LE.bind(callBuffer),
  int32_t: callBuffer.writeInt32LE.bind(callBuffer),
  uint32_t: callBuffer.writeUInt32LE.bind(callBuffer),
  int64_t: callBuffer.writeBigInt64LE.bind(callBuffer),
  uint64_t: callBuffer.writeBigUInt64LE.bind(callBuffer),
  float: callBuffer.writeFloatLE.bind(callBuffer),
  double: callBuffer.writeDoubleLE.bind(callBuffer)
};

const cbWriters = {
  int8_t: callBackBuffer.writeInt8.bind(callBackBuffer),
  uint8_t: callBackBuffer.writeUInt8.bind(callBackBuffer),
  int16_t: callBackBuffer.writeInt16LE.bind(callBackBuffer),
  uint16_t: callBackBuffer.writeUInt16LE.bind(callBackBuffer),
  int32_t: callBackBuffer.writeInt32LE.bind(callBackBuffer),
  uint32_t: callBackBuffer.writeUInt32LE.bind(callBackBuffer),
  int64_t: callBackBuffer.writeBigInt64LE.bind(callBackBuffer),
  uint64_t: callBackBuffer.writeBigUInt64LE.bind(callBackBuffer),
  float: callBackBuffer.writeFloatLE.bind(callBackBuffer),
  double: callBackBuffer.writeDoubleLE.bind(callBackBuffer)
};

const readers = {
  int8_t: callBuffer.readInt8.bind(callBuffer),
  uint8_t: callBuffer.readUInt8.bind(callBuffer),
  int16_t: callBuffer.readInt16LE.bind(callBuffer),
  uint16_t: callBuffer.readUInt16LE.bind(callBuffer),
  int32_t: callBuffer.readInt32LE.bind(callBuffer),
  uint32_t: callBuffer.readUInt32LE.bind(callBuffer),
  int64_t: callBuffer.readBigInt64LE.bind(callBuffer),
  uint64_t: callBuffer.readBigUInt64LE.bind(callBuffer),
  float: callBuffer.readFloatLE.bind(callBuffer),
  double: callBuffer.readDoubleLE.bind(callBuffer)
};

const cbReaders = {
  int8_t: callBackBuffer.readInt8.bind(callBackBuffer),
  uint8_t: callBackBuffer.readUInt8.bind(callBackBuffer),
  int16_t: callBackBuffer.readInt16LE.bind(callBackBuffer),
  uint16_t: callBackBuffer.readUInt16LE.bind(callBackBuffer),
  int32_t: callBackBuffer.readInt32LE.bind(callBackBuffer),
  uint32_t: callBackBuffer.readUInt32LE.bind(callBackBuffer),
  int64_t: callBackBuffer.readBigInt64LE.bind(callBackBuffer),
  uint64_t: callBackBuffer.readBigUInt64LE.bind(callBackBuffer),
  float: callBackBuffer.readFloatLE.bind(callBackBuffer),
  double: callBackBuffer.readDoubleLE.bind(callBackBuffer)
};

function firstPassNormalizeType (typ) {
  if (Array.isArray(typ)) {
    return [
      firstPassNormalizeType(typ[0]),
      typ[1].map(firstPassNormalizeType)
    ];
  }
  if (typ.includes('*')) return 'pointer';
  let unsigned = '';
  if (typ.startsWith('unsigned ')) {
    unsigned = 'u';
    typ = typ.replace('unsigned ', '');
  }
  if (typ in typSizes) {
    return `${unsigned}int${typSizes[typ] * 8}_t`;
  }
  return typ;
}

function createCbFromUserCb (userCb, [retType, argTypes]) {
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
  };
  const ptr = binding.createCallback(mapBinType(retType), argTypes.map(mapBinType), cb);
  return ptr;
}

const libCache = {};

function getNativeFunction (lib, func, ret, args) {
  ret = firstPassNormalizeType(ret);
  args = args.map(firstPassNormalizeType);
  const retBinTyp = mapBinType(ret);
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
  const callPtr = binding.addSignature(
    funcPtr,
    retBinTyp,
    args.map((x, i) => {
      if (Array.isArray(x)) {
        callbacks[i] = true;
        return 'pointer';
      } else {
        return x;
      }
    }).map(mapBinType)
  );

  const retSize = sizes[ret];
  const argWriters = args.map(x => writers[mapRWType(x)]);
  const getReturnVal = readers[mapRWType(ret)];

  return function (...callArgs) {
    let offset = 0;
    offset = callBuffer.writeBigUInt64LE(callPtr, offset);
    offset += retSize;
    for (let i = 0; i < args.length; i++) {
      if (callbacks[i]) {
        const userCb = callArgs[i];
        const cbPtr = createCbFromUserCb(userCb, args[i]);
        offset = callBuffer.writeBigUInt64LE(cbPtr, offset);
      } else {
        offset = argWriters[i](callArgs[i], offset);
      }
    }
    binding.call();
    if (retSize > 0) {
      return getReturnVal(8);
    }
  };
}

exports.getNativeFunction = getNativeFunction;
exports.getBufferPointer = binding.getBufPtr;
exports.sizeof = sizeof;
