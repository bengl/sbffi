#include "sbffi_dyncallback.h"
#include "dyncall_args.h"
#include "dyncall_types.h"
#include "js_native_api.h"
#include "node_api.h"
#include "node_api_types.h"
#include <string.h>
#include <assert.h>

const char * ASYNC_NAME = "sbffi:callback";

void * callBackBuffer;

napi_value js_setCallBackBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1);
  size_t len;
  napi_call(napi_get_buffer_info(env, args[0], &callBackBuffer, &len));
  return NULL;
}

// dyncall_disgnature.h
const char textSigValues[] = "BcsijlfdpvCSIJL";

char cbHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata) {
  cb_sig * sig = (cb_sig *)userdata;

  // [ ...args, ret]
  void * tempCallBackBuffer = malloc(sig->args_size + sig->ret_size);
  void * offset = tempCallBackBuffer;

  for (uint32_t i = 0; i < sig->argc; i++) {
    switch (sig->argv[i]) {
#define js_cb_handler_arg(enumTyp, typ, argFn) \
      case enumTyp:\
        *(typ *)offset = argFn(args);\
        offset = offset + sizeof(typ);\
        break;
      js_cb_handler_arg(fn_type_bool, DCbool, dcbArgBool)
      js_cb_handler_arg(fn_type_char, DCchar, dcbArgChar)
      js_cb_handler_arg(fn_type_short, DCshort, dcbArgShort)
      js_cb_handler_arg(fn_type_int, DCint, dcbArgInt)
      js_cb_handler_arg(fn_type_long, DClong, dcbArgLong)
      js_cb_handler_arg(fn_type_long_long, DClonglong, dcbArgLongLong)
      js_cb_handler_arg(fn_type_float, DCfloat, dcbArgFloat)
      js_cb_handler_arg(fn_type_double, DCdouble, dcbArgDouble)
      js_cb_handler_arg(fn_type_pointer, DCpointer, dcbArgPointer)
      case fn_type_void:
        exit(1); // TODO handle this better
      js_cb_handler_arg(fn_type_u_char, DCuchar, dcbArgUChar)
      js_cb_handler_arg(fn_type_u_short, DCushort, dcbArgUShort)
      js_cb_handler_arg(fn_type_u_int, DCuint, dcbArgUInt)
      js_cb_handler_arg(fn_type_u_long, DCulong, dcbArgULong)
      js_cb_handler_arg(fn_type_u_long_long, DCulonglong, dcbArgULongLong)
    }
  }

  // TODO set is_blocking based on return type
  cb_data *data = (cb_data *)malloc(sizeof(cb_data));
  data->buf = tempCallBackBuffer;
  data->len = sig->args_size + sig->ret_size;
  napi_status status;
  status = napi_acquire_threadsafe_function(sig->func);
  assert(status == napi_ok);
  status = napi_call_threadsafe_function(sig->func, data, true);
  assert(status == napi_ok);
  status = napi_release_threadsafe_function(sig->func, napi_tsfn_release);
  assert(status == napi_ok);

  // TODO the process stays alive after the callback is done. maybe we need to unref?

  switch(sig->return_type) {
#define js_cb_return_type(enumTyp, typ, sym) \
    case enumTyp:\
      result->sym = *(typ *)offset;\
      return textSigValues[enumTyp];
    js_cb_return_type(fn_type_bool, DCbool, B)
    js_cb_return_type(fn_type_char, DCchar, c)
    js_cb_return_type(fn_type_short, DCshort, s)
    js_cb_return_type(fn_type_int, DCint, i)
    js_cb_return_type(fn_type_long, DClong, j)
    js_cb_return_type(fn_type_long_long, DClonglong, l)
    js_cb_return_type(fn_type_float, DCfloat, f)
    js_cb_return_type(fn_type_double, DCdouble, d)
    js_cb_return_type(fn_type_pointer, DCpointer, p)
    case fn_type_void:
      return 'v';
    js_cb_return_type(fn_type_u_char, DCuchar, C)
    js_cb_return_type(fn_type_u_short, DCushort, S)
    js_cb_return_type(fn_type_u_int, DCuint, I)
    js_cb_return_type(fn_type_u_long, DCulong, J)
    js_cb_return_type(fn_type_u_long_long, DCulonglong, L)
  }
}

void call_js(napi_env env, napi_value js_cb, void * context, void * data) {
  cb_sig * sig = (cb_sig *)context;
  cb_data * cbData = (cb_data*)data;
  void * tempBuf = cbData->buf;
  memcpy(callBackBuffer, tempBuf, cbData->len);
  napi_status status;
  napi_value undefined;
  napi_call(napi_get_undefined(env, &undefined));
  napi_value result;
  napi_call(napi_call_function(env, undefined, js_cb, 0, NULL, &result));
  if (sig->return_type != fn_type_void) {
    memcpy(cbData->buf + sig->args_size, callBackBuffer + sig->args_size, sig->ret_size);
  }
  free(cbData);
}

size_t getTypeSize(fn_type typ) {
  switch(typ) {
    case fn_type_bool: return sizeof(DCbool);
    case fn_type_char: return sizeof(DCchar);
    case fn_type_short: return sizeof(DCshort);
    case fn_type_int: return sizeof(DCint);
    case fn_type_long: return sizeof(DClong);
    case fn_type_long_long: return sizeof(DClonglong);
    case fn_type_float: return sizeof(DCfloat);
    case fn_type_double: return sizeof(DCdouble);
    case fn_type_pointer: return sizeof(DCpointer);
    case fn_type_void: return 0;
    case fn_type_u_char: return sizeof(DCuchar);
    case fn_type_u_short: return sizeof(DCushort);
    case fn_type_u_int: return sizeof(DCuint);
    case fn_type_u_long: return sizeof(DCulong);
    case fn_type_u_long_long: return sizeof(DCulonglong);
  }
}

napi_value js_createCallback(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_get_args(3)
  napi_get_uint32(retTyp, args[0]);
  uint32_t fnArgc;
  napi_call(napi_get_array_length(env, args[1], &fnArgc));

  cb_sig * sig = (cb_sig *)malloc(sizeof(cb_sig) + sizeof(fn_type[fnArgc]));
  char * textSig = (char *)malloc(sizeof(char[fnArgc + 2]));
  sig->return_type = retTyp;
  sig->ret_size = getTypeSize(retTyp);
  sig->argc = fnArgc;
  sig->args_size = 0;
  for (uint32_t i = 0; i < fnArgc; i++) {
    napi_value key;
    napi_call(napi_create_uint32(env, i, &key));
    napi_value val;
    napi_call(napi_get_property(env, args[1], key, &val));
    napi_get_uint32(argTyp, val)
    sig->argv[i] = (fn_type)argTyp;
    sig->args_size += getTypeSize((fn_type)argTyp);
    textSig[i] = textSigValues[argTyp];
  }
  textSig[fnArgc] = ')';
  textSig[fnArgc+1] = textSigValues[retTyp];

  napi_value cbFunc = args[2];
  napi_threadsafe_function threadsafeFn;
  napi_value asyncName;
  napi_call(napi_create_string_utf8(env, ASYNC_NAME, strlen(ASYNC_NAME), &asyncName));
  napi_call(napi_create_threadsafe_function(env, cbFunc, NULL, asyncName, 0, 1, NULL, NULL, (void *)sig, &call_js, &sig->func));

  DCCallback * dcCb = dcbNewCallback(textSig, &cbHandler, sig);

  napi_return_uint64((uint64_t)dcCb)
}
