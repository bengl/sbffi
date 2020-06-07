#include "sbffi_dyncallback.hpp"
#include <node_api.h>
#include <string.h>
#include <cassert>

namespace sbffi {

const char * ASYNC_NAME = "sbffi:callback";

uint8_t * callBackBuffer;

void js_setCallBackBuffer(const CallbackInfo& info) {
  callBackBuffer = info[0].As<Buffer<uint8_t>>().Data();
}

// dyncall_disgnature.h
const char textSigValues[] = "BcsijlfdpvCSIJL";

char cbHandler(DCCallback* cb, DCArgs* args, DCValue* result, void* userdata) {
  cb_sig * sig = (cb_sig *)userdata;

  // [ ...args, ret]
  uint8_t * tempCallBackBuffer = (uint8_t *)malloc(sig->args_size + sig->ret_size);
  uint8_t * offset = tempCallBackBuffer;

  for (uint32_t i = 0; i < sig->argc; i++) {
    switch (sig->argv[i]) {
      case fn_type_void:
        exit(1); // TODO handle this better
#define js_cb_handler_arg(enumTyp, typ, _3, _4, argFn, _6) \
      case enumTyp:\
        *(typ *)offset = argFn(args);\
        offset = offset + sizeof(typ);\
        break;
      CALLBACK_TYPES_EXCEPT_VOID(js_cb_handler_arg)
    }
  }

  // TODO set is_blocking based on return type
  cb_data *data = (cb_data *)malloc(sizeof(cb_data));
  data->buf = tempCallBackBuffer;
  data->len = sig->args_size + sig->ret_size;
  napi_status status;
  status = napi_acquire_threadsafe_function(sig->func);
  assert(status == napi_ok);
  status = napi_call_threadsafe_function(sig->func, data, napi_tsfn_blocking);
  assert(status == napi_ok);
  status = napi_release_threadsafe_function(sig->func, napi_tsfn_release);
  assert(status == napi_ok);

  // TODO the process stays alive after the callback is done. maybe we need to unref?

  switch(sig->return_type) {
    case fn_type_void:
      return 'v';
#define js_cb_return_type(enumTyp, typ, _3, _4, _5, sym) \
    case enumTyp:\
      result->sym = *(typ *)offset;\
      return textSigValues[enumTyp];
    CALLBACK_TYPES_EXCEPT_VOID(js_cb_return_type)
  }
}

void call_js(napi_env env, napi_value js_cb, void * context, void * data) {
  cb_sig * sig = (cb_sig *)context;
  cb_data * cbData = (cb_data*)data;
  uint8_t * tempBuf = cbData->buf;
  memcpy(callBackBuffer, tempBuf, cbData->len);
  napi_status status;
  napi_value undefined;
  status = napi_get_undefined(env, &undefined);
  assert(status == napi_ok);
  napi_value result;
  status = napi_call_function(env, undefined, js_cb, 0, NULL, &result);
  assert(status == napi_ok);

  if (sig->return_type != fn_type_void) {
    memcpy(cbData->buf + sig->args_size, callBackBuffer + sig->args_size, sig->ret_size);
  }
  free(cbData);
}

size_t getTypeSize(fn_type typ) {
  switch(typ) {
    case fn_type_void: return 0;
#define get_size_type(enumTyp, typ, _3, _4, _5, _6)\
    case enumTyp: return sizeof(typ);
    CALLBACK_TYPES_EXCEPT_VOID(get_size_type)
  }
}

Value js_createCallback(const CallbackInfo& info) {
  Env env = info.Env();

  uint32_t retTyp = info[0].As<Number>().Uint32Value();
  Array args = info[1].As<Array>();
  uint32_t fnArgc = args.Length();

  cb_sig * sig = (cb_sig *)malloc(sizeof(cb_sig) + (sizeof(fn_type) * fnArgc));
  char * textSig = (char *)malloc(sizeof(char) * (fnArgc + 2));
  sig->return_type = (fn_type)retTyp;
  sig->ret_size = getTypeSize((fn_type)retTyp);
  sig->argc = fnArgc;
  sig->args_size = 0;
  for (uint32_t i = 0; i < fnArgc; i++) {
    Value key = Number::New(env, i);
    uint32_t argTyp = args.Get(key).As<Number>().Uint32Value();
    sig->argv[i] = (fn_type)argTyp;
    sig->args_size += getTypeSize((fn_type)argTyp);
    textSig[i] = textSigValues[argTyp];
  }
  textSig[fnArgc] = ')';
  textSig[fnArgc+1] = textSigValues[retTyp];

  // TODO rewrite this using C++ NAPI APIs.
  napi_status status;
  napi_value cbFunc = info[2];
  napi_threadsafe_function threadsafeFn;
  napi_value asyncName;
  status = napi_create_string_utf8(env, ASYNC_NAME, strlen(ASYNC_NAME), &asyncName);
  assert(status == napi_ok);
  status = napi_create_threadsafe_function(env, cbFunc, NULL, asyncName, 0, 1, NULL, NULL, (void *)sig, &call_js, &sig->func);
  assert(status == napi_ok);

  DCCallback * dcCb = dcbNewCallback(textSig, &cbHandler, sig);

  return BigInt::New(env, (uint64_t)dcCb);
}

}
