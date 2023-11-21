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

#define TOTAL_CORE 4

pthread_barrier_t bar;

typedef uint64_t ticks;

typedef void (*fp)(void *);

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

// Add test codes here, or we can use a separate file to test
// use #include to include the code we want to test
void my_loop(void *args) {
  int result = 0;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 12; ++j) {
      result += (i + j);
    }
  }

  return;
}

void my_loop2(void *args) {
  int result = 0;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 12; ++j) {
      for (int k = 0; k < 10; ++k)
        result += (i + j + k);
    }
  }

  return;
}

void my_loop3(void *args) {
  int result = 0;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 12; ++j) {
      for (int k = 0; k < 10; ++k)
        result += (i + j - k);
    }
  }

  return;
}

void my_loop4(void *args) {
  int result = 0;
  for (int i = 0; i < 114514; ++i) {
    if (result == 0) {
      result += 11;
    } else {
      result += i & 0xff;
    }
  }

  return;
}

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

int main(int argc, const char **argv) {
  // Change the TOTAL_CORE instead of here
  pthread_t p[TOTAL_CORE];
  pthread_barrier_init(&bar, NULL, TOTAL_CORE);

  void (*func[])(void *) = {my_loop, my_loop2, my_loop3, my_loop4};
  const char *funcname[] = {"my_loop", "my_loop2", "my_loop3", "my_loop4"};
  // Note: need to change the second dimension of args, due to the C standard
  uint64_t args[TOTAL_CORE][4] = {
      {
          (uint64_t)1,
          (uint64_t)funcname[0],
          (uint64_t)func[0],
          (uint64_t)NULL,
      },
      {
          (uint64_t)1,
          (uint64_t)funcname[1],
          (uint64_t)func[1],
          (uint64_t)NULL,
      },
      {
          (uint64_t)1,
          (uint64_t)funcname[2],
          (uint64_t)func[2],
          (uint64_t)NULL,
      },
      {
          (uint64_t)1,
          (uint64_t)funcname[3],
          (uint64_t)func[3],
          (uint64_t)NULL,
      },
  };

  cpu_set_t sets[TOTAL_CORE];

  for (int i = 0; i < TOTAL_CORE; ++i) {
    fprintf(stderr, "create thread %d\n", i);
    CPU_ZERO(&sets[i]);
    CPU_SET(i, &sets[i]);

    int rc = pthread_create(&p[i], NULL, thread_handler, (void *)args[i]);

    if (rc != 0) {
      fprintf(stderr, "%d\n", i);
      exit(EXIT_FAILURE);
    }

    rc = pthread_setaffinity_np(p[i], sizeof(cpu_set_t), &sets[i]);
    if (rc != 0) {
      fprintf(stderr, "Error calling pthread_setaffinity_np: %d\n", i);
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < TOTAL_CORE; ++i) {
    pthread_join(p[i], NULL);
  }
}