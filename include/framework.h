#ifndef __TEST_FRAME__
#define __TEST_FRAME__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "jsonobj.h"
#include <stdint.h>

#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t ticks;

typedef int (*fp)(void);

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
  // #ifdef __aarch64__
  //   // l1 cache miss
  //   uint64_t l1_i_miss;
  //   uint64_t l1_d_miss;
  //   // l2 cache miss
  //   uint64_t l2_miss;
  // #endif
  void *dll;
} func_args;

typedef struct {
  uint64_t size;
  uint64_t current;
  func_args *funcs;

  int cpu;
  // fd for perf_event_open
  // special case for perf_event_id = -1
  // use -1 for no perf_event and enable cycle counter
  int perf_event_id;
  struct perf_event_attr *attr;
} test_args;

/**
 * @brief  thread handler
 */
void *thread_handler(void *arg);

void start_test(uint64_t core, test_args *args);

void add_function(test_args *args, char *funcname, void *dll,
                  const char *dllname);

test_args *create_test_args(uint64_t core, int perf_event_id);

test_args *parse_from_json(const char *json_file, uint64_t *core,
                           int perf_event_id);

void free_test_args(uint64_t core, test_args *args);

void get_result(uint64_t core, test_args *args, uint64_t *memory);

void load_dll(uint64_t core, test_args *args, const char *dllname, void *dll);

json_node *create_result_json_array(const json_node *coreinfo,
                                    uint64_t *total_tasks);

void store_results(json_node *coreinfo, json_node *result, uint64_t *memory,
                   const char *item);

int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                    int group_fd, unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif