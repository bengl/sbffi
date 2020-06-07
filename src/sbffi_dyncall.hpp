#include "sbffi_common.hpp"
#include <dyncall.h>

namespace sbffi {

typedef struct fn_sig {
  DCCallVM * vm;
  void * fn;
  fn_type return_type;
  size_t argc;
  fn_type argv[];
} fn_sig;


void js_setCallBuffer(const CallbackInfo& info);
Value js_addSignature(const CallbackInfo& info);
void js_call(const CallbackInfo& info);
Value js_getBufPtr(const CallbackInfo& info);

}
