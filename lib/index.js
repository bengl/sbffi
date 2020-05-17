const dc = require('../build/Debug/sbffi.node');

const callBuffer = Buffer.alloc(1024);
dc.setCallBuffer(callBuffer);
const callBackBuffer = Buffer.alloc(1024);
dc.setCallBackBuffer(callBackBuffer);

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
] = [0,1,2,3,4,5,6,7,8,9];

const typeIds = {
  bool: fn_type_bool,
  char: fn_type_char,
  short: fn_type_short,
  int: fn_type_int,
  long: fn_type_long,
  long_long: fn_type_long_long,
  float: fn_type_float,
  double: fn_type_double,
  pointer: fn_type_pointer,
  void: fn_type_void,
};

const typeAliases = {
  uint8_t: 'char',
  uint16_t: 'short',
  uint32_t: 'int',
  uint64_t: 'long',
  int8_t: 'char',
  int16_t: 'short',
  int32_t: 'int',
  int64_t: 'long',
  pointer: 'long'
}

function mapBinType(type) {
  if (type in typeIds) {
    return typeIds[type];
  }
  if (type in typeAliases) {
    return typeIds[typeAliases[type]];
  }
  throw TypeError('invalid type: ' + type);
}

const sizes = {
  char: 1,
  uint8_t: 1,
  int8_t: 1,
  short: 2,
  uint16_t: 2,
  int16_t: 2,
  int: 4,
  uint32_t: 4,
  int32_t: 4,
  long: 8,
  uint64_t: 8,
  int64_t: 8,
  void: 0,
  pointer: 8
};

const rwTypeMap = {
  pointer: 'uint64_t'
};

function mapRWType(typ) {
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
};

const readers = {
  int8_t: callBuffer.readInt8.bind(callBuffer),
  uint8_t: callBuffer.readUInt8.bind(callBuffer),
  int16_t: callBuffer.readInt16LE.bind(callBuffer),
  uint16_t: callBuffer.readUInt16LE.bind(callBuffer),
  int16_t: callBuffer.readInt16LE.bind(callBuffer),
  uint32_t: callBuffer.readUInt32LE.bind(callBuffer),
  int64_t: callBuffer.readBigInt64LE.bind(callBuffer),
  uint64_t: callBuffer.readBigUInt64LE.bind(callBuffer),
};

const cbReaders = {
  int8_t: callBackBuffer.readInt8.bind(callBackBuffer),
  uint8_t: callBackBuffer.readUInt8.bind(callBackBuffer),
  int16_t: callBackBuffer.readInt16LE.bind(callBackBuffer),
  uint16_t: callBackBuffer.readUInt16LE.bind(callBackBuffer),
  int16_t: callBackBuffer.readInt16LE.bind(callBackBuffer),
  uint32_t: callBackBuffer.readUInt32LE.bind(callBackBuffer),
  int64_t: callBackBuffer.readBigInt64LE.bind(callBackBuffer),
  uint64_t: callBackBuffer.readBigUInt64LE.bind(callBackBuffer),
};

function firstPassNormalizeType(typ) {
  if (typ.includes('*')) return 'pointer';
  return typ;
}

function createCbFromUserCb(userCb, [retType, argTypes]) {
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
  }
  const ptr = dc.createCallback(mapBinType(retType), argTypes.map(mapBinType), cb);
  return ptr;
}

const libCache = {};

function getNativeFunction(lib, func, ret, args) {
  ret = firstPassNormalizeType(ret)
  args = args.map(firstPassNormalizeType)
  const retBinTyp = mapBinType(ret);
  let libPtr = libCache[lib];
  if (!libPtr) {
    libPtr = dc.dlLoadLibrary(lib);
    if (!libPtr) {
      throw new Error(`Library not found: ${lib}`);
    }
    libCache[lib] = libPtr;
  }
  const funcPtr = dc.dlFindSymbol(libPtr, func);
  if (!funcPtr) {
    throw new Error(`Function not found: ${func} in library: ${lib}`);
  }
  const callbacks = new Array(args.length);
  const callPtr = dc.addSignature(
    funcPtr,
    retBinTyp,
    args.map((x, i) => {
      if (Array.isArray(x)) {
        callbacks[i] = true;
        return 'uint64_t';
      } else {
        return x;
      }
    }).map(mapBinType)
  );

  const retSize = sizes[ret];
  const argWriters = args.map(x => writers[mapRWType(x)]);
  const getReturnVal = readers[mapRWType(ret)];

  return function(...callArgs) {
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
    dc.call();
    if (retSize > 0) {
      return getReturnVal(8);
    }
  };
}

exports.getNativeFunction = getNativeFunction;
exports.getBufferPointer = dc.getBufPtr;
