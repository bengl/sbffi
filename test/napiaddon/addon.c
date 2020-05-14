#include <node_api.h>
#define NAPI_VERSION 6
#include <assert.h>

napi_value add(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 2;
  napi_value args[2];
  status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
  assert(status == napi_ok);

  uint32_t a;
  status = napi_get_value_uint32(env, args[0], &a);
  assert(status == napi_ok);

  uint32_t b;
  status = napi_get_value_uint32(env, args[1], &b);
  assert(status == napi_ok);

  napi_value result;
  uint32_t sum = test_add_uint32_t(a, b);
  status = napi_create_uint32(env, sum, &result);
  assert(status == napi_ok);

  return result;
}

#define DECLARE_NAPI_METHOD(name, func)                                        \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor addDescriptor = DECLARE_NAPI_METHOD("add", add);
  status = napi_define_properties(env, exports, 1, &addDescriptor);
  assert(status == napi_ok);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
