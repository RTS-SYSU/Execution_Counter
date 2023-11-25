#ifndef __TEST_FRAME__
#define __TEST_FRAME__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ticks;

typedef void (*fp)(void *);

typedef enum {
  TYPE_INT_64,
  TYPE_FLOAT_32,
  TYPE_FLOAT_64,
  TYPE_POINTER,
} arg_type;

typedef union {
  int64_t int64;
  float float32;
  double float64;
  void *pointer;
} arg_val;

typedef struct {
  uint64_t arg_size;
  arg_val *arg_val;
} func_arg;

typedef struct {
  // Note that, func name will not be freed, please manage it by yourself
  const char *funcname;
  // function pointer
  fp funcptr;
  // function arguments, same as above, please manage it by yourself
  void *args;
  // ticks
  uint64_t result;
} func_args;

typedef struct {
  uint64_t size;
  uint64_t current;
  func_args *funcs;
} test_args;

/**
 * @brief  thread handler
 */
void *thread_handler(void *arg);

void start_test(uint64_t core, test_args *args);

void add_function(test_args *args,
                  const char *funcname,
                  fp funcptr,
                  void *funargc);

test_args *create_test_args(uint64_t core);

void free_test_args(test_args *args, uint64_t core);

void get_result(test_args *args, uint64_t core);

#ifdef __cplusplus
}
#endif

#endif