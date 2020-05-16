#include <dynload.h>
#include "sbffi_common.h"

napi_value js_dlLoadLibrary(napi_env env, napi_callback_info info);
napi_value js_dlFreeLibrary(napi_env env, napi_callback_info info);
napi_value js_dlFindSymbol(napi_env env, napi_callback_info info);
