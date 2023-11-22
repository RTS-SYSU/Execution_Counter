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
} test_args;

/**
 * @brief  thread handler
 */
void *thread_handler(void *arg);

void start_test(uint64_t core, test_args **args);

void create_arg(test_args *arg, const char *funcname, fp funcptr, void *args);
#ifdef __cplusplus
}
#endif

#endif