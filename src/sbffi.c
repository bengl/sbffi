#include <node_api.h>
#define NAPI_VERSION 6
#include "macros.h"

#include "sbffi_dyncallback.h"
#include "sbffi_dynload.h"
#include "sbffi_dyncall.h"

#define export_method(name, fn) {\
  napi_property_descriptor desc = { name, 0, fn, 0, 0, 0, napi_enumerable, 0 };\
  napi_call(napi_define_properties(env, exports, 1, &desc));\
  }

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;

  export_method("setCallBackBuffer", js_setCallBackBuffer);
  export_method("createCallback", js_createCallback);

  export_method("dlLoadLibrary", js_dlLoadLibrary);
  export_method("dlFreeLibrary", js_dlFreeLibrary);
  export_method("dlFindSymbol", js_dlFindSymbol);

  export_method("setCallBuffer", js_setCallBuffer);
  export_method("addSignature", js_addSignature);
  export_method("call", js_call);
  export_method("getBufPtr", js_getBufPtr);

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
