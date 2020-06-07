#ifndef SBFFI_COMMON_H
#define SBFFI_COMMON_H

#include <napi.h>
#include <cinttypes>

using namespace Napi;

namespace sbffi {

typedef enum fn_type {
  fn_type_bool,
  fn_type_char,
  fn_type_short,
  fn_type_int,
  fn_type_long,
  fn_type_long_long,
  fn_type_float,
  fn_type_double,
  fn_type_pointer,
  fn_type_void,
  fn_type_u_char,
  fn_type_u_short,
  fn_type_u_int,
  fn_type_u_long,
  fn_type_u_long_long,
} fn_type;

}

#define CALL_TYPES_EXCEPT_VOID(V)\
  V(fn_type_bool, DCbool, dcCallBool, dcArgBool, dcbArgBool, B)\
  V(fn_type_char, DCchar, dcCallChar, dcArgChar, dcbArgChar, c)\
  V(fn_type_short, DCshort, dcCallShort, dcArgShort, dcbArgShort, s)\
  V(fn_type_int, DCint, dcCallInt, dcArgInt, dcbArgInt, i)\
  V(fn_type_long, DClong, dcCallLong, dcArgLong, dcbArgLong, j)\
  V(fn_type_long_long, DClonglong, dcCallLongLong, dcArgLongLong, dcbArgLongLong, l)\
  V(fn_type_float, DCfloat, dcCallFloat, dcArgFloat, dcbArgFloat, f)\
  V(fn_type_double, DCdouble, dcCallDouble, dcArgDouble, dcbArgDouble, d)\
  V(fn_type_pointer, DCpointer, dcCallPointer, dcArgPointer, dcbArgPointer, p)
// TODO struct is missing from this ^

#define CALLBACK_TYPES_EXCEPT_VOID(V)\
  CALL_TYPES_EXCEPT_VOID(V)\
  V(fn_type_u_char, DCuchar, _, _, dcbArgUChar, C)\
  V(fn_type_u_short, DCushort, _, _, dcbArgUShort, S)\
  V(fn_type_u_int, DCuint, _, _, dcbArgUInt, I)\
  V(fn_type_u_long, DCulong, _, _, dcbArgULong, J)\
  V(fn_type_u_long_long, DCulonglong, _, _, dcbArgULongLong, L)\

#endif
