#include "dyncall.h"
#include "sbffi_common.h"
#include "sbffi_dyncallback.h"
#include "sbffi_dynload.h"
#include "sbffi_dyncall.h"

void finalize_sbffi_data(napi_env env, void * data, void * hint) {
  sbffi_data * sbffiData = (sbffi_data *)data;
  dcFree(sbffiData->vm);
  free(sbffiData);
}

NAPI_MODULE_INIT(/* napi_env env, napi_value exports*/) {
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
  // These types could in theory be of different sizes on different systems.
  // We get their size here to make it available in JS.
  export_size(char)
  export_size(unsigned char)
  export_size(short)
  export_size(unsigned short)
  export_size(int)
  export_size(unsigned int)
  export_size(long)
  export_size(unsigned long)
  export_size(long long)
  export_size(unsigned long long)
  export_size(size_t)
  export_size(char *)
  export_size(bool)
  export_size(uint8_t)
  export_size(int8_t)
  export_size(uint16_t)
  export_size(int16_t)
  export_size(uint32_t)
  export_size(int32_t)
  export_size(uint64_t)
  export_size(int64_t)
  export_size(float)
  export_size(double)
  napi_call(napi_set_named_property(env, exports, "sizes", sizes));

  sbffi_data * data = malloc(sizeof(sbffi_data));
  napi_call(napi_set_instance_data(env, data, finalize_sbffi_data, NULL));

  return exports;
}
