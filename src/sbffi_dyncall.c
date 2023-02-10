#include "sbffi_dyncall.h"
#include "js_native_api.h"

napi_value js_setCallBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1);
  size_t len;
  sbffi_data * sbData;
  napi_call(napi_get_instance_data(env, (void*)&sbData));
  napi_call(napi_get_buffer_info(env, args[0], (void **)&sbData->callBuffer, &len));

  // Initialize the DCCallVM whilr we're at it.
  sbData->vm = dcNewCallVM(256);
  dcMode(sbData->vm, DC_CALL_C_DEFAULT);
  dcReset(sbData->vm);

  return NULL;
}

napi_value js_addSignature(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(3);
  void * fnPtr;
  napi_get_ptr(fnPtr,args[0])
  uint32_t retTyp;
  napi_call(napi_get_value_uint32(env, args[1], &retTyp));
  uint32_t fnArgc;
  napi_call(napi_get_array_length(env, args[2], &fnArgc));

  fn_sig * sig = (fn_sig *)malloc(sizeof(fn_sig) + (sizeof(fn_type) * fnArgc));
  sig->fn = fnPtr;
  sig->return_type = retTyp;
  sig->argc = fnArgc;
  for (uint32_t i = 0; i < fnArgc; i++) {
    uint32_t argTyp;
    napi_value key;
    napi_call(napi_create_uint32(env, i, &key));
    napi_value val;
    napi_call(napi_get_property(env, args[2], key, &val));
    napi_call(napi_get_value_uint32(env, val, &argTyp));
    sig->argv[i] = (fn_type)argTyp;
  }

  napi_return_uint64((uint64_t)sig)
}

#define call_to_buf(_1, dcTyp, dcFn, _4, _5, _6) void call_##dcFn(DCCallVM * vm, uint8_t * callBuffer, DCpointer funcptr) {\
  dcTyp * retVal = (dcTyp*)(callBuffer + 8);\
  *retVal = dcFn(vm, funcptr);\
}
call_types_except_void(call_to_buf)
void call_dcCallVoid(DCCallVM * vm, uint8_t * callBuffer, DCpointer funcptr) {
  dcCallVoid(vm, funcptr);
}

napi_value js_call(napi_env env, napi_callback_info info) {
  napi_status status;

  sbffi_data * sbData;
  napi_call(napi_get_instance_data(env, (void*)&sbData));
  uint8_t * callBuffer = sbData->callBuffer;
  DCCallVM * vm = sbData->vm;

  fn_sig * sig = *(fn_sig **)callBuffer;
  uint8_t * offset = callBuffer + sizeof(fn_sig *);
  void (*callFn)(DCCallVM *, uint8_t *, DCpointer);
  switch (sig->return_type) {
    case fn_type_void:
      callFn = &call_dcCallVoid;
      break;
#define js_call_ret_case(enumTyp, typ, callFunc, _4, _5, _6) \
    case enumTyp: \
      callFn = &call_##callFunc;\
      offset += sizeof(typ);\
      break;
    call_types_except_void(js_call_ret_case)
    default:
      napi_throw_type_error(env, NULL, "invalid types in signature");
      return NULL;
  }

  for (size_t i = 0; i < sig->argc; i++) {
    fn_type typ = sig->argv[i];
    switch (typ) {
      case fn_type_void:
        napi_throw_type_error(env, NULL, "cannot have void argument");
        return NULL;
#define js_call_arg_case(enumTyp, argTyp, _3, argFunc, _5, _6) \
      case enumTyp:\
        argFunc(vm, *(argTyp*)offset);\
        offset += sizeof(argTyp);\
        break;
      call_types_except_void(js_call_arg_case)
      default:
        napi_throw_type_error(env, NULL, "invalid types in signature");
        return NULL;
    }
  }

  callFn(vm, callBuffer, (DCpointer)sig->fn);
  dcReset(vm);

  return NULL;
}

napi_value js_getBufPtr(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1);
  void * buffer;
  size_t len;
  napi_call(napi_get_buffer_info(env, args[0], &buffer, &len));
  napi_return_uint64((uint64_t)buffer)
}
