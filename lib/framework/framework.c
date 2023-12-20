#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void get_result(uint64_t core, test_args *args, uint64_t id) {
  for (uint64_t i = 0; i < core; ++i) {
    // func_args *current = args[i].funcs;
    json_node *result_handler = args[i].result_handler;
    if (result_handler->child == NULL) {
      result_handler->child = create_json_node();
      result_handler->child->type = JSON_OBJECT;
      result_handler = result_handler->child;
      args[i].result_handler = result_handler;
    } else {
      result_handler->next = create_json_node();
      result_handler->next->type = JSON_OBJECT;
      result_handler = result_handler->next;
      args[i].result_handler = result_handler;
    }
    for (uint64_t j = 0; j < args[i].current; ++j) {
      // fprintf(stderr,
      //         "Core: %lu, function: %p, function name: %s, ticks: %lu\n", i,
      //         (fp)current[j].funcptr, (char *)current[j].funcname,
      //         current[j].results);
      if (j == 0) {
        // id field
        result_handler->child = create_json_node();
        result_handler->child->type = JSON_INT;
        result_handler->child->key = TO_JSON_STRING("id");
        result_handler->child->val.val_as_int = id;
        // tasks field
        result_handler->child->next = create_json_node();
        result_handler->child->next->type = JSON_ARRAY;
        result_handler->child->next->key = TO_JSON_STRING("tasks");
        result_handler->child->next->child = create_json_node();
        result_handler->child->next->child->type = JSON_OBJECT;
        result_handler = result_handler->child->next->child;
      }
      // function field and ticks field
      result_handler->child = create_json_node();
      result_handler->child->type = JSON_STRING;
      result_handler->child->key = TO_JSON_STRING("function");
      result_handler->child->val.val_as_str =
          TO_JSON_STRING((char *)args[i].funcs[j].funcname);
      result_handler->child->next = create_json_node();
      result_handler->child->next->type = JSON_INT;
      result_handler->child->next->key = TO_JSON_STRING("ticks");
      result_handler->child->next->val.val_as_int = args[i].funcs[j].results;
      if (j != args[i].current - 1) {
        result_handler->next = create_json_node();
        result_handler->next->type = JSON_OBJECT;
        result_handler = result_handler->next;
      }
    }
  }
}

test_args *parse_from_json(const char *json_file, const char *dllname,
                           void *dll, uint64_t *cores, json_node *results) {
  json_node *root = parse_json_file(json_file);
  if (root == NULL) {
    fprintf(stderr, "Failed to parse json file: %s\n", json_file);
    exit(EXIT_FAILURE);
  }

  // void *dll = dlopen(libname, RTLD_NOW | RTLD_GLOBAL);
  // if (dll == NULL) {
  //   fprintf(stderr, "Failed to open %s\n", libname);
  //   exit(EXIT_FAILURE);
  // }

  if (results == NULL) {
    fprintf(stderr, "result should be an initialized json_array\n");
    exit(EXIT_FAILURE);
  }

  results->type = JSON_ARRAY;

  fp f = NULL;

  json_node *core = root->child;

  *cores = 0;

  while (core != NULL) {
    ++(*cores);
    core = core->next;
  }

  test_args *args = create_test_args(*cores);

  core = root->child;

  json_node *object = NULL;
  for (uint64_t i = 0; i < *cores; ++i) {
    if (i == 0) {
      // get child
      results->child = create_json_node();
      results->child->type = JSON_OBJECT;
      object = results->child;
      // results->child->child = create_json_node();
      // results->child->child->type = JSON_INT;
    } else {
      object->next = create_json_node();
      object->next->type = JSON_OBJECT;
      object = object->next;
    }
    object->child = create_json_node();
    object->child->type = JSON_INT;
    object->child->key = TO_JSON_STRING("core");
    object->child->val.val_as_int = i;
    object->child->next = create_json_node();
    object->child->next->type = JSON_ARRAY;
    object->child->next->key = TO_JSON_STRING("results");
    args[i].result_handler = object->child->next;

    uint64_t cur = json_get(core, "core")->val.val_as_int;
    json_node *tasks = json_get(core, "tasks");
    json_node *task = json_get(tasks, "tasks")->child;
    while (task != NULL) {
      char *funcname =
          TO_JSON_STRING(json_get(task, "function")->val.val_as_str);
      f = dlsym(dll, funcname);
      if (f == NULL) {
        fprintf(stderr, "Unable to find func: %s in %s\n", funcname, dllname);
        exit(EXIT_FAILURE);
      }
      // TODO: maybe we can add an arguments later?
      add_function(args + cur, funcname, f, NULL);
      task = task->next;
    }
    core = core->next;
  }

  free_json_node(root);
  return args;
}

void *reload_dll(const char *dllname, uint64_t core, test_args *args,
                 void *dll) {
  for (uint64_t i = 0; i < core; ++i) {
    test_args *cargs = args + i;
    for (uint64_t j = 0; j < args[i].current; ++i) {
      cargs->funcs[j].funcptr = NULL;
    }
  }

  dlclose(dll);

  void *dll_new = dlopen(dllname, RTLD_NOW | RTLD_GLOBAL);

  if (dll == NULL) {
    fprintf(stderr, "Unable to open dll %s\n", dllname);
    exit(EXIT_FAILURE);
  }

  // for (uint64_t i = 0; i < args->current; ++i) {
  //   char *funcname = args->funcs[i].funcname;
  //   fp f = dlsym(dll, funcname);
  //   if (f == NULL) {
  //     fprintf(stderr, "Unable to find func: %s in %s\n", funcname, dllname);
  //     exit(EXIT_FAILURE);
  //   }
  //   args->funcs[i].funcptr = f;
  // }

  for (uint64_t i = 0; i < core; ++i) {
    test_args *cargs = args + i;
    for (uint64_t j = 0; j < args[i].current; ++j) {
      char *funcname = cargs->funcs[j].funcname;
      fp f = dlsym(dll_new, funcname);
      if (f == NULL) {
        fprintf(stderr, "Unable to find func: %s in %s\n", funcname, dllname);
        exit(EXIT_FAILURE);
      }
      cargs->funcs[j].funcptr = f;
    }
  }

  return dll_new;
}