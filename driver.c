#include "framework.h"
#include "funcdef.h"
#include "jsonobj.h"
#include "jsonparser.h"
#include <dlfcn.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// May be we can do some json parsing here, so that we do not need to recompile
// every time
const char *helpMsg = "Usage: %s <repeats> <input_json> <output_json>\n";

#define LIB_NAME "libtestfunc.so"

#define SHARED_MEMORY_SIZE 4096

int main(int argc, const char **argv) {
  if (argc < 2 || argc != 4) {
    fprintf(stderr, helpMsg, argv[0]);
    exit(EXIT_FAILURE);
  }
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    fprintf(stderr, helpMsg, argv[0]);
    return 0;
  }

  uint64_t repeats = atoi(argv[1]);
  fflush(stdout);
  fflush(stdin);
  fflush(stderr);

  // Create a 4K shared memory using mmap
  // This is used to store the results
  uint64_t *memory =
      (uint64_t *)mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  uint64_t total_tasks = 0;
  json_node *coreinfo = parse_json_file(argv[2]);
  json_node *result = create_result_json_array(coreinfo, &total_tasks);

  for (uint64_t i = 0; i < repeats; ++i) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }

    if (pid == 0) {
      // Child process

      // Make the child process run in real-time priority
      struct sched_param param;
      param.sched_priority = sched_get_priority_max(SCHED_FIFO);
      pid_t pid = getpid();
      if (sched_setscheduler(pid, SCHED_FIFO, &param) != 0) {
        fprintf(stderr, "Failed to set scheduler\n");
        fprintf(stderr, "Please run this program as root\n");
        exit(EXIT_FAILURE);
      }
      void *dll = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
      if (dll == NULL) {
        fprintf(stderr, "Unable to open dll %s\n", LIB_NAME);
        exit(EXIT_FAILURE);
      }

      uint64_t core = 0;

      test_args *args = parse_from_json(argv[2], &core);
      load_dll(core, args, LIB_NAME, dll);
      start_test(core, args);
      get_result(core, args, memory);

      // for (uint64_t i = 0; i < core; ++i) {
      //   fprintf(stderr, "Core %lu\n", i);
      //   for (uint64_t j = 0; j < args[i].current; ++j) {
      //     fprintf(stderr, "Calling %s\n", args[i].funcs[j].funcname);
      //     fprintf(stderr, "Args: %p\n", args[i].funcs[j].args);
      //     fprintf(stderr, "Funcptr: %p\n", args[i].funcs[j].funcptr);
      //     fprintf(stderr, "Results: %lu\n", args[i].funcs[j].results);
      //   }
      // }

      free_test_args(core, args);

      dlclose(dll);
      exit(EXIT_SUCCESS);
    } else {
      int status = 0;
      wait(&status);
      if (status != 0) {
        fprintf(stderr, "Child process exited with status %d\n", status);
        exit(EXIT_FAILURE);
      }

      // fprintf(stderr, "Total tasks: %lu\n", total_tasks);

      // for (int i = 0; i < total_tasks; ++i) {
      //   fprintf(stderr, "Task %d: %lu\n", i, memory[i]);
      // }

      store_results(coreinfo, result, memory);
    }
  }

  munmap(memory, SHARED_MEMORY_SIZE);

  FILE *output = fopen(argv[3], "w");
  print_json(result, 0, output);
  fclose(output);
  free_json_node(coreinfo);
  free_json_node(result);
  return 0;
}
