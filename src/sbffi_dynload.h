#include <dynload.h>
#include <node_api.h>
#define NAPI_VERSION 6
#include "macros.h"

napi_value js_dlLoadLibrary(napi_env env, napi_callback_info info);
napi_value js_dlFreeLibrary(napi_env env, napi_callback_info info);
napi_value js_dlFindSymbol(napi_env env, napi_callback_info info);
