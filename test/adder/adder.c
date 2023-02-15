#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define mk_adder(t) mk_adder_type(t, t)

#define mk_adder_type(t, tname) typedef struct {\
    t a;\
    t b;\
    void (*cb)(t);\
  } thread_data_##tname;\
  t test_add_##tname(t a, t b) {\
    return a + b;\
  }\
  void test_add_ptr_##tname(t * a, t * b, t * r) {\
    *r = *a + *b;\
  }\
  void *add_thread_##tname(void * args) {\
    thread_data_##tname * data = (thread_data_##tname *)args;\
    t result = data->a + data->b;\
    void (*cb)(t) = data->cb;\
    free(data);\
    cb(result);\
    return NULL;\
  }\
  void test_add_async_##tname(t a, t b, void (*cb)(t)) {\
    pthread_t thread;\
    thread_data_##tname * args = (thread_data_##tname*)malloc(sizeof(thread_data_##tname));\
    args->a = a;\
    args->b = b;\
    args->cb = cb;\
    pthread_create(&thread, NULL, add_thread_##tname, (void*)args);\
  }\
  void test_add_async_twice_##tname(t a, t b, void (*cb)(t)) {\
    test_add_async_##tname(a, b, cb);\
    test_add_async_##tname(a, b, cb);\
  }\

mk_adder(int)
mk_adder_type(unsigned int, unsigned_int)
mk_adder(short)
mk_adder_type(unsigned short, unsigned_short)
mk_adder(long)
mk_adder_type(unsigned long, unsigned_long)
mk_adder_type(long long, long_long)
mk_adder_type(unsigned long long, unsigned_long_long)
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
