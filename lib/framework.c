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

#if defined(__x86_64__)
static __inline__ __attribute__((always_inline)) ticks getCPUCycle() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));

  return ((ticks)hi << 32) | lo;
}
#endif

#ifdef __aarch64__
// TODO: implement this, for aarch64
static __inline__ __attribute__((always_inline)) ticks getCPUCycle() {
  return 0;
}
#endif

// how to create the args
// args[0]: a uint64_t, how many function to run
// then is the function name, pointer and it arguments
void *thread_handler(void *args) {
  // use the barrier to make sure all threads are ready
  pthread_barrier_wait(&bar);

  uint64_t total = ((uint64_t *)args)[0];
  for (uint64_t i = 0; i < total; ++i) {
    // get the base address of the current function
    uint64_t *current = &((uint64_t *)args)[i * 3 + 1];
    // function name
    const char *functionName = (const char *)((uint64_t *)current)[0];
    // function pointer
    fp func = (void (*)(void *))((uint64_t *)current)[1];
    // function arguments
    void *arg = (void *)((uint64_t *)current)[2];
    ticks start, end;
    start = getCPUCycle();
    func(arg);
    end = getCPUCycle();
    fprintf(stderr, "function: %p, function name: %s, ticks: %lu\n", func,
            functionName, end - start);
  }
  return (void *)0;
}

void start_test(uint64_t core, test_args **args) {
  pthread_t threads[core];

  pthread_barrier_init(&bar, NULL, core);

  cpu_set_t sets[core];

  for (uint64_t i = 0; i < core; ++i) {
    // fprintf(stderr, "create thread %d\n", i);
    CPU_ZERO(&sets[i]);
    CPU_SET(i, &sets[i]);

    int rc = pthread_create(&threads[i], NULL, thread_handler, (void *)args[i]);
    if (rc != 0) {
      fprintf(stderr, "%lu\n", i);
      exit(EXIT_FAILURE);
    }

    rc = pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &sets[i]);
    if (rc != 0) {
      fprintf(stderr, "Error calling pthread_setaffinity_np: %lu\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (uint64_t i = 0; i < core; ++i) {
    pthread_join(threads[i], NULL);
  }
}

void create_arg(test_args *arg, const char *funcname, fp funcptr, void *args) {
  arg->funcname = (uint64_t)funcname;
  arg->funcptr = (uint64_t)funcptr;
  arg->args = (uint64_t)args;
}