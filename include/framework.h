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

typedef struct {
  uint64_t funcname;
  uint64_t funcptr;
  uint64_t args;
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

void free_test_args(test_args *args);

#ifdef __cplusplus
}
#endif

#endif