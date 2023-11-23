#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "framework.h"

pthread_barrier_t bar;

#ifdef __x86_64__
static __inline__ __attribute__((always_inline)) ticks getCPUCycle() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));

  return ((ticks)hi << 32) | lo;
}
#endif

#ifdef __aarch64__
static __inline__ __attribute__((always_inline)) ticks getCPUCycle() {
  uint64_t val;
  __asm__ __volatile__("mrs %0, pmccntr_el0" : "=r"(val));
  return val;
}
#endif

void *thread_handler(void *thread_args) {
  // use the barrier to make sure all threads are ready
  pthread_barrier_wait(&bar);

  test_args *args = (test_args *)thread_args;
  uint64_t total = args->current;
  func_args *current = args->funcs;

  for (uint64_t i = 0; i < total; ++i) {
    // get the base address of the current function
    // function name
    const char *functionName = (const char *)current->funcname;
    // function pointer
    fp func = (fp)current->funcptr;
    // function arguments
    void *arg = (void *)current->args;
    // fprintf(stderr, "function: %p, function name: %s\n", func, functionName);

    ticks start, end;
    start = getCPUCycle();
    func(arg);
    end = getCPUCycle();
    fprintf(stderr, "function: %p, function name: %s, ticks: %lu\n", func,
            functionName, end - start);
  }
  return (void *)0;
}

void start_test(uint64_t core, test_args *args) {
  pthread_barrier_init(&bar, NULL, core);
  pthread_t threads[core];
  cpu_set_t sets[core];

  for (uint64_t i = 0; i < core; ++i) {
    CPU_ZERO(&sets[i]);
    CPU_SET(i, &sets[i]);

    int rc =
        pthread_create(&threads[i], NULL, thread_handler, (void *)(args + i));
    if (rc != 0) {
      fprintf(stderr, "Error calling pthread_create.(Thread num: %lu)\n", i);
      exit(EXIT_FAILURE);
    }

    rc = pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &sets[i]);
    if (rc != 0) {
      fprintf(stderr,
              "Error calling pthread_setaffinity_np.(Thread num: %lu)\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (uint64_t i = 0; i < core; ++i) {
    pthread_join(threads[i], NULL);
  }
}

void set_arg(func_args *arg, const char *funcname, fp funcptr, void *args) {
  arg->funcname = (uint64_t)funcname;
  arg->funcptr = (uint64_t)funcptr;
  arg->args = (uint64_t)args;
}

void add_function(test_args *args,
                  const char *funcname,
                  fp funcptr,
                  void *funargc) {
  if (args->current >= args->size) {
    if (args->size != 0)
      args->size *= 2;
    else
      args->size = 1;
    args->funcs =
        (func_args *)realloc(args->funcs, sizeof(func_args) * args->size);
  }
  set_arg(&args->funcs[args->current++], funcname, funcptr, funargc);
  return;
}

test_args *create_test_args(uint64_t count) {
  test_args *args = (test_args *)malloc(sizeof(test_args) * count);
  if (args == NULL) {
    fprintf(stderr, "malloc failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  args->size = 0;
  args->current = 0;
  args->funcs = NULL;
  return args;
}

void free_test_args(test_args *args) {
  free(args->funcs);
  free(args);
}