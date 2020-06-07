#include <stdlib.h>
#include <dyncall_callback.h>
#include "sbffi_common.hpp"

namespace sbffi {

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

void js_setCallBackBuffer(const CallbackInfo& info);
Value js_createCallback(const CallbackInfo& info);

}
