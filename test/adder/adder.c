#include <stdint.h>
#include <stdio.h>

#define mk_adder(t) t test_add_##t(t a, t b) {\
    return a + b;\
  }\
  void test_add_ptr_##t(t * a, t * b, t * r) {\
    *r = *a + *b;\
  }\
  void test_add_async_##t(t a, t b, void (*cb)(t)) {\
    cb(a + b);\
  }\

mk_adder(uint8_t)
mk_adder(uint16_t)
mk_adder(uint32_t)
mk_adder(uint64_t)
mk_adder(int8_t)
mk_adder(int16_t)
mk_adder(int32_t)
mk_adder(int64_t)
