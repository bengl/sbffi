#include "node_api_types.h"
#include <stdlib.h>
#include <node_api.h>
#define NAPI_VERSION 6
#include <dyncall_callback.h>
#include "macros.h"

typedef struct cb_sig {
  napi_threadsafe_function func;
  fn_type return_type;
  size_t ret_size;
  size_t args_size;
  size_t argc;
  fn_type argv[];
} cb_sig;

typedef struct cb_data {
  void * buf;
  size_t len;
} cb_data;

napi_value js_setCallBackBuffer(napi_env env, napi_callback_info info);
napi_value js_createCallback(napi_env env, napi_callback_info info);
