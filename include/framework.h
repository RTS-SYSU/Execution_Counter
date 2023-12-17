#ifndef __TEST_FRAME__
#define __TEST_FRAME__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>
#include "jsonobj.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ticks;

typedef void (*fp)(void *);

// TODO: May be we can enable this feature in the future
// typedef enum {
//   TYPE_INT_64,
//   TYPE_FLOAT_32,
//   TYPE_FLOAT_64,
//   TYPE_POINTER,
// } arg_type;

// typedef union {
//   int64_t int64;
//   float float32;
//   double float64;
//   void *pointer;
// } arg_val;

// typedef struct {
//   uint64_t arg_size;
//   arg_val *arg_val;
// } func_arg;

typedef struct {
  // Note that, func name will be freed by framework
  char *funcname;
  // function pointer
  fp funcptr;
  // function arguments, same as above, please manage it by yourself
  void *args;
  // ticks
  uint64_t results;
} func_args;

typedef struct {
  uint64_t size;
  uint64_t current;
  func_args *funcs;
  // json handler, not freed by framework
  json_node *result_handler;
} test_args;

/**
 * @brief  thread handler
 */
void *thread_handler(void *arg);

void start_test(uint64_t core, test_args *args);

void add_function(test_args *args, char *funcname, fp funcptr, void *funargc);

test_args *create_test_args(uint64_t core);

test_args *parse_from_json(const char *json_file, const char *dllname,
                           void *dll, uint64_t *core, json_node *result);

void free_test_args(uint64_t core, test_args *args);

void get_result(uint64_t core, test_args *args, uint64_t count);

#ifdef __cplusplus
}
#endif

#endif