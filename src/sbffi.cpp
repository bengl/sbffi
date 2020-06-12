#include <iostream>
#include "v8.h"
#include <stdint.h>
#include <string.h>
#include <node.h>
#include <dlfcn.h>
#include <libtcc.h>

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
#include <v8-fast-api-calls.h>

#include "sbffi_common.h"

#define tcc_dir CMAKE_CURRENT_SOURCE_DIR
#define str(x) #x

using namespace v8;

uint8_t * callBuffer;

const char * argList [16] = {
  "arg0",
  "arg1",
  "arg2",
  "arg3",
  "arg4",
  "arg5",
  "arg6",
  "arg7",
  "arg8",
  "arg9",
  "arg10",
  "arg11",
  "arg12",
  "arg13",
  "arg14",
  "arg15"
};

char* getString(Isolate * isolate, Local<Value> value) {
  Local<String> string = value.As<String>();
  size_t storage = string->Utf8Length(isolate);
  char * cStr = (char *)calloc(storage+1, sizeof(char));
  const int flags =
      String::NO_NULL_TERMINATION | String::REPLACE_INVALID_UTF8;
  value.As<String>()->WriteUtf8(isolate, cStr, storage, nullptr, flags);
  return cStr;
}

void createFunctionString(char * dest, char * fnName, char * retType, uint32_t fnArgc, char ** fnArgv) {
  strcat(dest, "#include <stdint.h>\n");
  strcat(dest, retType);
  strcat(dest, " ");
  strcat(dest, fnName);
  strcat(dest, "(");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, fnArgv[i]);
    strcat(dest, " ");
    strcat(dest, argList[i]);
    if (i != fnArgc - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, ");\n");
  strcat(dest,
      "void sbffi_fn_call_"
      );
  strcat(dest, fnName);
  strcat(dest,
      "(uint8_t * callBuffer){\n"
      "uint8_t * cursor = callBuffer + sizeof("
  );
  strcat(dest, retType);
  strcat(dest, ");\n");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, fnArgv[i]);
    strcat(dest, " ");
    strcat(dest, argList[i]);
    strcat(dest, " = *(");
    strcat(dest, fnArgv[i]);
    strcat(dest,
        " *)cursor;\n"
        "cursor += sizeof("
    );
    strcat(dest, fnArgv[i]);
    strcat(dest, ");\n");
  }
  strcat(dest, retType);
  strcat(dest, "* retVal = (");
  strcat(dest, retType);
  strcat(dest,
      "*)callBuffer;\n"
      "*retVal = "
  );
  strcat(dest, fnName);
  strcat(dest, "(");
  for (uint32_t i = 0; i < fnArgc; i++) {
    strcat(dest, argList[i]);
    if (i != fnArgc - 1) {
      strcat(dest, ",");
    }
  }
  strcat(dest, ");\n}");
  // printf(">>>>SOURCE\n%s\n<<<<SOURCE\n", dest);
}

uint8_t * createFunction(char * fnString, char * fnName, void * fnPtr) {
  TCCState * tccState = tcc_new();
  tcc_set_lib_path(tccState, TCC_DIR);
  tcc_set_output_type(tccState, TCC_OUTPUT_MEMORY);
  tcc_compile_string(tccState, fnString);
  tcc_add_symbol(tccState, fnName, fnPtr);
  void * mem = malloc(tcc_relocate(tccState, NULL));
  tcc_relocate(tccState, mem);
  char * symName = (char *)calloc(15 + strlen(fnName), sizeof(char));
  strcat(symName, "sbffi_fn_call_");
  strcat(symName, fnName);
  uint8_t * retVal = (uint8_t *)tcc_get_symbol(tccState, symName);
  free(symName);
  tcc_delete(tccState);
  return retVal;
}

void js_setCallBuffer(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  auto buf = args[0].As<ArrayBuffer>();
  callBuffer = (uint8_t *)buf->GetContents().Data();
}

// createFunction(fnName, fnPtr, retType, [...argTypes])
void js_createFunction(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  char * fnName = getString(isolate, args[0]);
  void * fnPtr = (void *)args[1].As<BigInt>()->Uint64Value();
  char * retTyp = getString(isolate, args[2]);
  Local<Array> argList = args[3].As<Array>();
  uint32_t fnArgc = argList->Length();

  char ** fnArgv = (char **)malloc(sizeof(char *) * fnArgc);

  for (uint32_t i = 0; i < fnArgc; i++) {
    char * argTyp = getString(isolate, argList->Get(context, i).ToLocalChecked());
    fnArgv[i] = (char *)malloc(sizeof(char) * strlen(argTyp));
    memcpy(fnArgv[i], argTyp, strlen(argTyp));
  }

  char * fnSource = (char *)calloc(4096, sizeof(char));
  createFunctionString(fnSource, fnName, retTyp, fnArgc, fnArgv);
  for (uint32_t i = 0; i < fnArgc; i++) {
    free(fnArgv[i]);
  }
  free(fnArgv);

  uint8_t * newFnPtr = createFunction(fnSource, fnName, fnPtr);
  free(fnSource);

  args.GetReturnValue().Set(BigInt::NewFromUnsigned(isolate, (uint64_t)newFnPtr));
}

void makeFnCall() {
  void * fnPtr = *(void**)callBuffer;
  void (*fnToCall)(uint8_t *) = (void (*)(uint8_t *))fnPtr;
  fnToCall(callBuffer + 8);
}

void js_slowcall(const FunctionCallbackInfo<Value>& args) {
  makeFnCall();
}

void js_fastcall() {
  makeFnCall();
}

void js_loadLibrary(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  char * libName = getString(isolate, args[0]);
  void * libPtr = dlopen(libName, RTLD_NOW);
  args.GetReturnValue().Set(BigInt::NewFromUnsigned(isolate, (uint64_t)libPtr));
}

void js_findSymbol(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  void * libPtr = (void *)args[0].As<BigInt>()->Uint64Value();
  char * symName = getString(isolate, args[1]);
  void * sym = dlsym(libPtr, symName);
  args.GetReturnValue().Set(BigInt::NewFromUnsigned(isolate, (uint64_t)sym));
}

void Initialize(Local<Object> exports) {
  Isolate* isolate = exports->GetIsolate();
  Local<Context> context = isolate->GetCurrentContext();
  auto cfunc = CFunction::Make(js_fastcall);
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
  exports->Set(context, String::NewFromUtf8(isolate, "call").ToLocalChecked(), func).ToChecked();

#define export_method(name, fn) NODE_SET_METHOD(exports, name, fn)
  export_method("setCallBuffer", js_setCallBuffer);
  export_method("createFunction", js_createFunction);
  export_method("loadLibrary", js_loadLibrary);
  export_method("findSymbol", js_findSymbol);

  Local<Object> sizes = Object::New(isolate);
#define export_size(typ) {\
  sizes->Set(context, String::NewFromUtf8(isolate, #typ).ToLocalChecked(), Number::New(isolate, (double)sizeof(typ))).ToChecked();\
}

  export_size(char)
  export_size(short)
  export_size(int)
  export_size(long)
  export_size(long long)
  export_size(size_t)
  export_size(char *)

  exports->Set(context, String::NewFromUtf8(isolate, "sizes").ToLocalChecked(), sizes).ToChecked();

}

NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

