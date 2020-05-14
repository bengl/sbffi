#include "sbffi_dyncall.h"

void * callBuffer;

napi_value js_setCallBuffer(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1);
  size_t len;
  napi_call(napi_get_buffer_info(env, args[0], &callBuffer, &len));
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

  fn_sig * sig = (fn_sig *)malloc(sizeof(fn_sig) + sizeof(fn_type[fnArgc]));
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

#define callToBuf(dcFn, dcTyp) void call_##dcFn(DCCallVM * vm, DCpointer funcptr) {\
  dcTyp * retVal = (dcTyp*)(callBuffer + 8);\
  *retVal = dcFn(vm, funcptr);\
}

callToBuf(dcCallBool, DCbool)
callToBuf(dcCallChar, DCchar)
callToBuf(dcCallShort, DCshort)
callToBuf(dcCallInt, DCint)
callToBuf(dcCallLong, DClong)
callToBuf(dcCallLongLong, DClonglong)
callToBuf(dcCallFloat, DCfloat)
callToBuf(dcCallDouble, DCdouble)
callToBuf(dcCallPointer, DCpointer)

napi_value js_call(napi_env env, napi_callback_info info) {
  fn_sig * sig = *(fn_sig **)callBuffer;
  void * origOffset = callBuffer;
  void * offset = callBuffer + sizeof(fn_sig *);
  void (*callFn)(DCCallVM *, DCpointer);
  switch (sig->return_type) {
#define js_call_ret_case(callFunc, enumTyp, typ) \
    case enumTyp: {\
      callFn = &callFunc;\
      if (enumTyp != fn_type_void) {\
        offset += sizeof(typ);\
      }\
      break;}
    js_call_ret_case(dcCallVoid, fn_type_void, DCvoid)
    js_call_ret_case(call_dcCallBool, fn_type_bool, DCbool)
    js_call_ret_case(call_dcCallChar, fn_type_char, DCchar)
    js_call_ret_case(call_dcCallShort, fn_type_short, DCshort)
    js_call_ret_case(call_dcCallInt, fn_type_int, DCint)
    js_call_ret_case(call_dcCallLong, fn_type_long, DClong)
    js_call_ret_case(call_dcCallLongLong, fn_type_long_long, DClonglong)
    js_call_ret_case(call_dcCallFloat, fn_type_float, DCfloat)
    js_call_ret_case(call_dcCallDouble, fn_type_double, DCdouble)
    js_call_ret_case(call_dcCallPointer, fn_type_pointer, DCpointer)
  }

  DCCallVM * vm = dcNewCallVM(4096);
  dcMode(vm, DC_CALL_C_DEFAULT);
  dcReset(vm);

  for (size_t i = 0; i < sig->argc; i++) {
    fn_type typ = sig->argv[i];
    switch (typ) {
      case fn_type_void:
        napi_throw_type_error(env, NULL, "cannot have void argument");
        return NULL;
#define js_call_arg_case(argFunc, enumTyp, argTyp) \
      case enumTyp:\
        argFunc(vm, *(argTyp*)offset);\
        offset += sizeof(argTyp);\
        break;
      js_call_arg_case(dcArgBool, fn_type_bool, DCbool)
      js_call_arg_case(dcArgChar, fn_type_char, DCchar)
      js_call_arg_case(dcArgShort, fn_type_short, DCshort)
      js_call_arg_case(dcArgInt, fn_type_int, DCint)
      js_call_arg_case(dcArgLong, fn_type_long, DClong)
      js_call_arg_case(dcArgLongLong, fn_type_long_long, DClonglong)
      js_call_arg_case(dcArgFloat, fn_type_float, DCfloat)
      js_call_arg_case(dcArgDouble, fn_type_double, DCdouble)
      js_call_arg_case(dcArgPointer, fn_type_pointer, DCpointer)
    }
  }

  callFn(vm, (DCpointer)sig->fn);
  dcFree(vm);

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

