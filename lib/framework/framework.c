#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "framework.h"
#include "jsonobj.h"
#include "jsonparser.h"

pthread_barrier_t bar;

#ifdef __x86_64__
static inline __attribute__((always_inline)) ticks get_CPU_Cycle() {
  uint32_t lo, hi;
  asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

  return ((ticks)hi << 32) | lo;
}
#endif

#ifdef __aarch64__
static inline __attribute__((always_inline)) ticks get_CPU_Cycle() {
  uint64_t val;
  asm volatile("mrs %0, pmccntr_el0" : "=r"(val));
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
    // function pointer
    fp func = (fp)current->funcptr;
    // function arguments
    void *arg = (void *)current->args;
    // fprintf(stderr, "function: %p, function name: %s\n", func, functionName);

    ticks start, end;
    start = get_CPU_Cycle();
    func(arg);
    end = get_CPU_Cycle();
    current->results = (end - start);
    // may be we should not print here, due to we have to switch to the kernel
    // from user space
    // fprintf(stderr, "Core: %lu, function: %p, function name: %s, ticks :
    // %lu\n",
    //         args->core, func, functionName, end - start);
    current++;
  }
  return (void *)0;
}

void start_test(uint64_t core, test_args *args) {
  pthread_barrier_init(&bar, NULL, core);
  pthread_t threads[core];
  cpu_set_t sets[core];
  // struct sched_param param[core];

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

    // Make the thread be a real-time thread
    // param[i].sched_priority = sched_get_priority_max(SCHED_FIFO);
    // rc = pthread_setschedparam(threads[i], SCHED_FIFO, &param[i]);
    // if (rc != 0) {
    //   fprintf(stderr, "Error calling pthread_setschedparam.(Thread num:
    //   %lu)\n",
    //           i);
    //   exit(EXIT_FAILURE);
    // }
  }

  for (uint64_t i = 0; i < core; ++i) {
    pthread_join(threads[i], NULL);
  }

  // get_result(args, core);
}

void set_arg(func_args *arg, char *funcname, fp funcptr, void *args) {
  arg->funcname = funcname;
  arg->funcptr = funcptr;
  arg->args = args;
  arg->results = 0;
}

void add_function(test_args *args, char *funcname, fp funcptr, void *funargc) {
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

  memset(args, 0, sizeof(test_args) * count);
  return args;
}

void free_test_args(uint64_t core, test_args *args) {
  for (uint64_t i = 0; i < core; ++i) {
    for (uint64_t j = 0; j < args[i].current; ++j) {
      free(args[i].funcs[j].funcname);
      free(args[i].funcs[j].args);
    }
    free(args[i].funcs);
  }
  free(args);
}

void get_result(uint64_t core, test_args *args, uint64_t *memory) {
  size_t idx = 0;
  for (uint64_t i = 0; i < core; ++i) {
    for (uint64_t j = 0; j < args[i].current; ++j) {
      memory[idx++] = args[i].funcs[j].results;
    }
  }
}

test_args *parse_from_json(const char *json_file, uint64_t *cores) {
  json_node *root = parse_json_file(json_file);
  if (root == NULL) {
    fprintf(stderr, "Failed to parse json file: %s\n", json_file);
    exit(EXIT_FAILURE);
  }

  json_node *core = root->child;

  *cores = 0;

  while (core != NULL) {
    ++(*cores);
    core = core->next;
  }

  test_args *args = create_test_args(*cores);

  core = root->child;

  for (uint64_t i = 0; i < *cores; ++i) {
    uint64_t cur = json_get(core, "core")->val.val_as_int;
    json_node *tasks = json_get(core, "tasks");
    json_node *task = json_get(tasks, "tasks")->child;
    while (task != NULL) {
      char *funcname =
          TO_JSON_STRING(json_get(task, "function")->val.val_as_str);
      // TODO: maybe we can add an arguments later?
      add_function(args + cur, funcname, NULL, NULL);
      task = task->next;
    }
    core = core->next;
  }

  free_json_node(root);
  return args;
}

void load_dll(uint64_t core, test_args *args, const char *dllname, void *dll) {
  for (uint64_t i = 0; i < core; ++i) {
    test_args *cargs = args + i;
    for (uint64_t j = 0; j < args[i].current; ++j) {
      char *funcname = cargs->funcs[j].funcname;
      fp f = dlsym(dll, funcname);
      if (f == NULL) {
        fprintf(stderr, "Unable to find func: %s in %s\n", funcname, dllname);
        exit(EXIT_FAILURE);
      }
      cargs->funcs[j].funcptr = f;
    }
  }
}

json_node *create_result_json_array(const json_node *coreinfo,
                                    uint64_t *total_tasks) {
  if (coreinfo == NULL) {
    fprintf(stderr, "coreinfo can not be NULL\n");
    exit(EXIT_FAILURE);
  }
  json_node *result = create_json_node();
  result->type = JSON_ARRAY;
  json_node *object = NULL;

  json_node *core = coreinfo->child;

  while (core) {
    if (core == coreinfo->child) {
      // the first node
      result->child = create_json_node();
      result->child->type = JSON_OBJECT;
      object = result->child;
    } else {
      object->next = create_json_node();
      object->next->type = JSON_OBJECT;
      object = object->next;
    }
    object->child = create_json_node();
    object->child->type = JSON_INT;
    object->child->key = TO_JSON_STRING("core");
    object->child->val.val_as_int = core->child->val.val_as_int;
    object->child->next = create_json_node();
    object->child->next->type = JSON_ARRAY;
    object->child->next->key = TO_JSON_STRING("results");

    json_node *tasks = json_get(core, "tasks")->child;
    while (tasks != NULL) {
      tasks = tasks->next;
      ++(*total_tasks);
    }
    core = core->next;
  }

  return result;
}

void store_results(json_node *coreinfo, json_node *result, uint64_t *memory) {
  if (coreinfo == NULL || result == NULL) {
    fprintf(stderr, "coreinfo and result can not be NULL\n");
    exit(EXIT_FAILURE);
  }

  json_node *core = coreinfo->child;
  json_node *object = result->child;
  size_t idx = 0;

  while (core) {
    json_node *tasks = json_get(core, "tasks")->child;
    json_node *results = json_get(object, "results")->child;

    if (results == NULL) {
      json_node *tmp = json_get(object, "results");
      tmp->child = create_json_node();
      tmp->child->type = JSON_OBJECT;
      tmp->child->child = create_json_node();
      tmp->child->child->type = JSON_INT;
      tmp->child->child->key = TO_JSON_STRING("id");
      tmp->child->child->val.val_as_int = 0;
      tmp->child->child->next = create_json_node();
      tmp->child->child->next->type = JSON_ARRAY;
      tmp->child->child->next->key = TO_JSON_STRING("tasks");
      results = json_get(object, "results")->child;
    } else {
      uint64_t id = 1;
      while (results->next != NULL) {
        results = results->next;
        ++id;
      }
      results->next = create_json_node();
      results->next->type = JSON_OBJECT;
      results->next->child = create_json_node();
      results->next->child->type = JSON_INT;
      results->next->child->key = TO_JSON_STRING("id");
      results->next->child->val.val_as_int = id;
      results->next->child->next = create_json_node();
      results->next->child->next->type = JSON_ARRAY;
      results->next->child->next->key = TO_JSON_STRING("tasks");
      results = results->next;
    }
    json_node *handler = json_get(results, "tasks");
    while (tasks) {
      char *funcname = json_get(tasks, "function")->val.val_as_str;
      if (handler->child == NULL) {
        handler->child = create_json_node();
        handler->child->type = JSON_OBJECT;
        handler->child->child = create_json_node();
        handler->child->child->type = JSON_STRING;
        handler->child->child->key = TO_JSON_STRING("function");
        handler->child->child->val.val_as_str = TO_JSON_STRING(funcname);
        handler->child->child->next = create_json_node();
        handler->child->child->next->type = JSON_INT;
        handler->child->child->next->key = TO_JSON_STRING("ticks");
        handler->child->child->next->val.val_as_int = memory[idx++];
        handler = handler->child;
      } else {
        handler->next = create_json_node();
        handler->next->type = JSON_OBJECT;
        handler->next->child = create_json_node();
        handler->next->child->type = JSON_STRING;
        handler->next->child->key = TO_JSON_STRING("function");
        handler->next->child->val.val_as_str = TO_JSON_STRING(funcname);
        handler->next->child->next = create_json_node();
        handler->next->child->next->type = JSON_INT;
        handler->next->child->next->key = TO_JSON_STRING("ticks");
        handler->next->child->next->val.val_as_int = memory[idx++];
        handler = handler->next;
      }
      tasks = tasks->next;
    }

    core = core->next;
    object = object->next;
  }
}