#include <stdlib.h>
#include <dyncall_callback.h>
#include "sbffi_common.h"

typedef struct cb_sig {
  napi_threadsafe_function func;
  fn_type return_type;
  size_t ret_size;
  size_t args_size;
  size_t argc;
  fn_type argv[];
} cb_sig;

typedef struct cb_data {
  uint8_t * buf;
  size_t len;
} cb_data;

napi_value js_setCallBackBuffer(napi_env env, napi_callback_info info);
napi_value js_createCallback(napi_env env, napi_callback_info info);
