#include "js_native_api.h"
#include "js_native_api_types.h"
#include <assert.h>
#include <stdio.h>
#ifndef MACROS_H
#define MACROS_H

#define napi_call(expr) \
  status = expr;\
  if (status != napi_ok) {\
    const napi_extended_error_info * info;\
    status = napi_get_last_error_info(env, &info);\
    if (status != napi_ok) {\
      abort();\
    }\
    printf("napi_error: %s", info->error_message);\
    abort();\
  }\

#define napi_get_args(num) \
  size_t argc = num;\
  napi_value args[num];\
  napi_call(napi_get_cb_info(env, info, &argc, args, NULL, NULL));\
  if (argc != num) {\
    napi_throw_type_error(env, NULL, "expected #num args");\
    return NULL;\
  }

#define napi_get_uint32(name, expr) \
  uint32_t name;\
  napi_call(napi_get_value_uint32(env, expr, &name));

#define napi_get_string(name,expr,len) \
  char name[len];\
  {\
  size_t strLen;\
  napi_call(napi_get_value_string_utf8(env, expr, name, len, &strLen));\
  if (strLen >= len) {\
    napi_throw_type_error(env, NULL, "string must be less than #len");\
    return NULL;\
  }\
  }

#define napi_get_ptr(name,expr) {\
  bool lossless;\
  napi_call(napi_get_value_bigint_uint64(env, expr, (uint64_t*)&name, &lossless));\
  assert(lossless);\
  }

#define napi_return_uint64(expr) {\
  napi_value returnVal;\
  napi_call(napi_create_bigint_uint64(env, expr, &returnVal));\
  return returnVal;\
  }

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

#endif
