#include <stdlib.h>
#include <string.h>
#include <dyncall.h>
#include "sbffi_common.h"

typedef struct fn_sig {
  void * fn;
  fn_type return_type;
  size_t argc;
  fn_type argv[];
} fn_sig;

napi_value js_setCallBuffer(napi_env env, napi_callback_info info);
napi_value js_addSignature(napi_env env, napi_callback_info info);
napi_value js_call(napi_env env, napi_callback_info info);
napi_value js_getBufPtr(napi_env env, napi_callback_info info);
