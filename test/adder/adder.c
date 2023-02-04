#include <stdint.h>
#include <stdio.h>

#define mk_adder(t) mk_adder_type(t, t)

#define mk_adder_type(t, tname) t test_add_##tname(t a, t b) {\
    return a + b;\
  }\
  void test_add_ptr_##tname(t * a, t * b, t * r) {\
    *r = *a + *b;\
  }\
  void test_add_async_##tname(t a, t b, void (*cb)(t)) {\
    cb(a + b);\
  }\
  void test_add_async_twice_##tname(t a, t b, void (*cb)(t)) {\
    cb(a + b);\
    cb(a + b);\
  }\

mk_adder(int)
mk_adder(short)
mk_adder(long)
mk_adder_type(long long, long_long)
mk_adder(uint8_t)
mk_adder(uint16_t)
mk_adder(uint32_t)
mk_adder(uint64_t)
mk_adder(int8_t)
mk_adder(int16_t)
mk_adder(int32_t)
mk_adder(int64_t)
mk_adder(float)
mk_adder(double)
