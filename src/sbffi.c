#include "sbffi_common.h"
#include "sbffi_dyncallback.h"
#include "sbffi_dynload.h"
#include "sbffi_dyncall.h"

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;

#define export_method(name, fn) {\
  napi_property_descriptor desc = { name, 0, fn, 0, 0, 0, napi_enumerable, 0 };\
  napi_call(napi_define_properties(env, exports, 1, &desc));\
  }

  export_method("setCallBackBuffer", js_setCallBackBuffer);
  export_method("createCallback", js_createCallback);

  export_method("dlLoadLibrary", js_dlLoadLibrary);
  export_method("dlFreeLibrary", js_dlFreeLibrary);
  export_method("dlFindSymbol", js_dlFindSymbol);

  export_method("setCallBuffer", js_setCallBuffer);
  export_method("addSignature", js_addSignature);
  export_method("call", js_call);
  export_method("getBufPtr", js_getBufPtr);

  napi_value sizes;
  napi_call(napi_create_object(env, &sizes));
#define export_size(typ) {\
  napi_value sizeof_val;\
  napi_call(napi_create_uint32(env, sizeof(typ), &sizeof_val));\
  napi_call(napi_set_named_property(env, sizes, #typ, sizeof_val));\
  }
  export_size(char)
  export_size(short)
  export_size(int)
  export_size(long)
  export_size(long long)
  export_size(size_t)
  export_size(char *)

  napi_call(napi_set_named_property(env, exports, "sizes", sizes));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
