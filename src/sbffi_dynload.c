#include "sbffi_dynload.h"

bool napi_is_null(napi_env env, napi_value value) {
  napi_status status;
  napi_valuetype valuetype;
  napi_call(napi_typeof(env, value, &valuetype));
  return valuetype == napi_null;
}

napi_value js_dlLoadLibrary(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1)
  napi_get_string(libName, args[0], 256)
  DLLib * libPtr = dlLoadLibrary(libName);
  napi_return_uint64((uint64_t)libPtr)
}

napi_value js_dlFreeLibrary(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(1)
  DLLib * libPtr;
  napi_get_ptr(libPtr,args[0])
  dlFreeLibrary(libPtr);
  return NULL;
}

napi_value js_dlFindSymbol(napi_env env, napi_callback_info info) {
  napi_status status;
  napi_get_args(2)
  DLLib * libPtr = NULL;
  if (!napi_is_null(env, args[0])) {
    napi_get_ptr(libPtr, args[0])
  }
  napi_get_string(symName, args[1], 256)
  void * sym = dlFindSymbol(libPtr, symName);
  napi_return_uint64((uint64_t)sym)
}
