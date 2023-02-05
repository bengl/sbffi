#include "dynload.h"
#include "v8-container.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-typed-array.h"
#include <cstdint>
#include <iostream>
#include <node.h>

#ifdef __GNUC__
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#else
#define UNLIKELY(expr) expr
#endif
#define CHECK(expr)                                                           \
  do {                                                                        \
    if (UNLIKELY(!(expr))) {                                                  \
      std::cout << #expr << "\n";                                             \
      abort();                                                            \
    }                                                                         \
  } while (0)
#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_LT(a, b) CHECK((a) < (b))

#include "v8-fast-api-calls.h"
#include <dyncall.h>
#include "sbffi_common.h"

using namespace v8;

namespace sbffi {

typedef struct fn_sig {
  DCCallVM * vm;
  void * fn;
  fn_type return_type;
  size_t argc;
  fn_type argv[];
} fn_sig;

uint8_t * callBuffer;

void js_setCallBuffer(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  auto buf = args[0].As<ArrayBuffer>();
  callBuffer = (uint8_t *)buf->GetBackingStore()->Data();
}

void js_addSignature(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  void * fnPtr = (void *)args[0].As<v8::BigInt>()->Uint64Value();
  uint32_t retTyp = args[1].As<v8::Number>()->Uint32Value(context).ToChecked();
  uint32_t fnArgc = args[2].As<v8::Array>()->Length();

  fn_sig * sig = (fn_sig *)malloc(sizeof(fn_sig) + (sizeof(fn_type) * fnArgc));
  sig->fn = fnPtr;
  sig->return_type = (fn_type)retTyp;
  sig->argc = fnArgc;
  for (uint32_t i = 0; i < fnArgc; i++) {
    sig->argv[i] = (fn_type)args[2].As<v8::Array>()->Get(context, i).ToLocalChecked().As<v8::Number>()->Uint32Value(context).ToChecked();
  }

  DCCallVM * vm = dcNewCallVM(256);
  dcMode(vm, DC_CALL_C_DEFAULT);
  dcReset(vm);
  sig->vm = vm;

  args.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(isolate, (uint64_t)sig));
}

#define call_to_buf(_1, dcTyp, dcFn, _4, _5, _6) void call_##dcFn(DCCallVM * vm, DCpointer funcptr) {\
  dcTyp * retVal = (dcTyp*)(callBuffer + 8);\
  *retVal = dcFn(vm, funcptr);\
}
call_types_except_void(call_to_buf)

void makeFnCall() {
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
    call_types_except_void(js_call_ret_case)
    default:
      abort();
  }

  for (size_t i = 0; i < sig->argc; i++) {
    fn_type typ = sig->argv[i];
    switch (typ) {
      case fn_type_void:
        abort();
#define js_call_arg_case(enumTyp, argTyp, _3, argFunc, _5, _6) \
      case enumTyp:\
        argFunc(sig->vm, *(argTyp*)offset);\
        offset += sizeof(argTyp);\
        break;
      call_types_except_void(js_call_arg_case)
      default:
        abort();
    }
  }

  callFn(sig->vm, (DCpointer)sig->fn);
  dcReset(sig->vm);

  return;
}

void js_slowcall(const FunctionCallbackInfo<Value>& args) {
  makeFnCall();
}

void js_fastcall(v8::Local<v8::Value> receiver) {
  makeFnCall();
}

CFunction cfunc(CFunction::Make(js_fastcall));

void js_getBufPtr(const FunctionCallbackInfo<Value>& args) {
  args.GetReturnValue().Set(
      v8::BigInt::NewFromUnsigned(
        args.GetIsolate(),
        (uint64_t)args[0].As<Uint8Array>()->Buffer()->GetBackingStore()->Data()
      )
  );
}

void js_dlLoadLibrary(const FunctionCallbackInfo<Value>& args) {
  v8::String::Utf8Value utf8val(args.GetIsolate(), args[0]);
  args.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(args.GetIsolate(), (uint64_t)dlLoadLibrary(*utf8val)));
}

void js_dlFreeLibrary(const FunctionCallbackInfo<Value>& args) {
  dlFreeLibrary((DLLib *)args[0].As<BigInt>()->Uint64Value());
}

void js_dlFindSymbol(const FunctionCallbackInfo<Value>& args) {
  DLLib * libPtr = NULL;
  if (!args[0]->IsNull()) {
    libPtr = (DLLib *)args[0].As<BigInt>()->Uint64Value();
  }
  v8::String::Utf8Value symName(args.GetIsolate(), args[1]);
  void * sym = dlFindSymbol(libPtr, *symName);
  args.GetReturnValue().Set(v8::BigInt::NewFromUnsigned(args.GetIsolate(), (uint64_t)sym));
}

void Initialize(Local<Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();

  auto funcTmpl = v8::FunctionTemplate::New(isolate,
      js_slowcall,
      Local<Value>(),
      Local<v8::Signature>(),
      0,
      v8::ConstructorBehavior::kThrow,
      v8::SideEffectType::kHasNoSideEffect,
      &cfunc);
  auto func = funcTmpl
    ->GetFunction(context)
    .ToLocalChecked();
  exports->Set(context, String::NewFromUtf8(isolate, "call").ToLocalChecked(), func);

  NODE_SET_METHOD(exports, "setCallBuffer", js_setCallBuffer);

  auto sizes = v8::Object::New(isolate);
  sizes->Set(context, String::NewFromUtf8(isolate, "char").ToLocalChecked(), v8::Number::New(isolate, sizeof(char)));
  sizes->Set(context, String::NewFromUtf8(isolate, "short").ToLocalChecked(), v8::Number::New(isolate, sizeof(short)));
  sizes->Set(context, String::NewFromUtf8(isolate, "int").ToLocalChecked(), v8::Number::New(isolate, sizeof(int)));
  sizes->Set(context, String::NewFromUtf8(isolate, "long").ToLocalChecked(), v8::Number::New(isolate, sizeof(long)));
  sizes->Set(context, String::NewFromUtf8(isolate, "long long").ToLocalChecked(), v8::Number::New(isolate, sizeof(long long)));
  sizes->Set(context, String::NewFromUtf8(isolate, "size_t").ToLocalChecked(), v8::Number::New(isolate, sizeof(size_t)));
  sizes->Set(context, String::NewFromUtf8(isolate, "char *").ToLocalChecked(), v8::Number::New(isolate, sizeof(char *)));
  exports->Set(context, String::NewFromUtf8(isolate, "sizes").ToLocalChecked(), sizes);

  NODE_SET_METHOD(exports, "addSignature", js_addSignature);

  NODE_SET_METHOD(exports, "getBufPtr", js_getBufPtr);

  NODE_SET_METHOD(exports, "dlLoadLibrary", js_dlLoadLibrary);

  NODE_SET_METHOD(exports, "dlFreeLibrary", js_dlFreeLibrary);

  NODE_SET_METHOD(exports, "dlFindSymbol", js_dlFindSymbol);

#define export_size(typ) \

}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}
