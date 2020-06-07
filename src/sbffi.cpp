#include "sbffi_dyncall.hpp"
#include "sbffi_dynload.hpp"
#include "sbffi_dyncallback.hpp"

namespace sbffi {

Object Init(Env env, Object exports) {

#define EXPORT_FUNCTION(name, fn) \
  exports.Set(String::New(env, name), Function::New(env, fn))

  EXPORT_FUNCTION("setCallBackBuffer", js_setCallBackBuffer);
  EXPORT_FUNCTION("createCallback", js_createCallback);

  EXPORT_FUNCTION("dlLoadLibrary", js_dlLoadLibrary);
  EXPORT_FUNCTION("dlFreeLibrary", js_dlFreeLibrary);
  EXPORT_FUNCTION("dlFindSymbol", js_dlFindSymbol);

  EXPORT_FUNCTION("setCallBuffer", js_setCallBuffer);
  EXPORT_FUNCTION("addSignature", js_addSignature);
  EXPORT_FUNCTION("call", js_call);
  EXPORT_FUNCTION("getBufPtr", js_getBufPtr);

  Object sizes = Object::New(env);
#define EXPORT_SIZE(typ) \
  sizes.Set(String::New(env, #typ), Number::New(env, sizeof(typ)));
  EXPORT_SIZE(char)
  EXPORT_SIZE(short)
  EXPORT_SIZE(int)
  EXPORT_SIZE(long)
  EXPORT_SIZE(long long)
  EXPORT_SIZE(size_t)
  EXPORT_SIZE(char *)

  exports.Set(String::New(env, "sizes"), sizes);

  return exports;
}

NODE_API_MODULE(sbffi, Init)

}
