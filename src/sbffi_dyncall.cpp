#include "sbffi_dyncall.hpp"
#include "napi.h"

namespace sbffi {

uint8_t * callBuffer;

void js_setCallBuffer(const CallbackInfo& info) {
  callBuffer = info[0].As<Buffer<uint8_t>>().Data();
}

Value js_addSignature(const CallbackInfo& info) {
  Env env = info.Env();
  bool lossless;
  void * fnPtr = (void *)info[0].As<BigInt>().Uint64Value(&lossless);
  uint32_t retTyp = info[1].As<Number>().Uint32Value();
  Array args = info[2].As<Array>();
  uint32_t fnArgc = args.Length();

  fn_sig * sig = (fn_sig *)malloc(sizeof(fn_sig) + (sizeof(fn_type) * fnArgc));
  sig->fn = fnPtr;
  sig->return_type = (fn_type)retTyp;
  sig->argc = fnArgc;

  for (uint32_t i = 0; i < fnArgc; i++) {
    Value key = Number::New(env, i);
    uint32_t argTyp = args.Get(key).As<Number>().Uint32Value();
    sig->argv[i] = (fn_type)argTyp;
  }

  DCCallVM * vm = dcNewCallVM(256);
  dcMode(vm, DC_CALL_C_DEFAULT);
  dcReset(vm);
  sig->vm = vm;

  return BigInt::New(env, (uint64_t)sig);
}

#define call_to_buf(_1, dcTyp, dcFn, _4, _5, _6) void call_##dcFn(DCCallVM * vm, DCpointer funcptr) {\
  dcTyp * retVal = (dcTyp*)(callBuffer + 8);\
  *retVal = dcFn(vm, funcptr);\
}
CALL_TYPES_EXCEPT_VOID(call_to_buf)

void js_call(const CallbackInfo& info) {
  Env env = info.Env();
  fn_sig * sig = *(fn_sig **)callBuffer;
  uint8_t * origOffset = callBuffer;
  uint8_t * offset = callBuffer + sizeof(fn_sig *);
  void (*callFn)(DCCallVM *, DCpointer);
  switch (sig->return_type) {
    case fn_type_void:
      callFn = &dcCallVoid;
      break;
#define js_call_ret_case(enumTyp, typ, callFunc, _4, _5, _6) \
    case enumTyp: \
      callFn = &call_##callFunc;\
      offset += sizeof(typ);\
      break;
    CALL_TYPES_EXCEPT_VOID(js_call_ret_case)
    default:
      TypeError::New(env, "invalid types in signature").ThrowAsJavaScriptException();
      return;
  }

  for (size_t i = 0; i < sig->argc; i++) {
    fn_type typ = sig->argv[i];
    switch (typ) {
      case fn_type_void:
        TypeError::New(env, "cannot have void argument").ThrowAsJavaScriptException();
        return;
#define js_call_arg_case(enumTyp, argTyp, _3, argFunc, _5, _6) \
      case enumTyp:\
        argFunc(sig->vm, *(argTyp*)offset);\
        offset += sizeof(argTyp);\
        break;
      CALL_TYPES_EXCEPT_VOID(js_call_arg_case)
      default:
        TypeError::New(env, "invalid types in signature").ThrowAsJavaScriptException();
        return;
    }
  }

  callFn(sig->vm, (DCpointer)sig->fn);
  dcReset(sig->vm);
}

Value js_getBufPtr(const CallbackInfo& info) {
  Env env = info.Env();
  return BigInt::New(env, (uint64_t)info[0].As<Buffer<uint8_t>>().Data());
}

}
