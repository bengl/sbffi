#include <dynload.h>
#include "sbffi_common.hpp"

namespace sbffi {

Value js_dlLoadLibrary(const CallbackInfo& info);
void js_dlFreeLibrary(const CallbackInfo& info);
Value js_dlFindSymbol(const CallbackInfo& info);

}
