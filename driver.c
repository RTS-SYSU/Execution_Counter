#include "framework.h"
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

const char *helpMsg = "Usage: %s <repeats> <input_json> <output_json>\n";

#define LIB_NAME "libtestfunc.so"

#define SHARED_MEMORY_SIZE 4096

int main(int argc, const char **argv) {
  if (argc < 2 || argc != 4) {
    fprintf(stderr, helpMsg, argv[0]);
    exit(EXIT_FAILURE);
  }
  if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    fprintf(stderr, helpMsg, argv[0]);
    return 0;
  }

  uint64_t repeats = atoi(argv[1]);

  // Create a 4K shared memory using mmap
  // This is used to store the results
  uint64_t *memory =
      (uint64_t *)mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  memset(memory, 0, SHARED_MEMORY_SIZE);
  uint64_t total_tasks = 0;
  json_node *coreinfo = parse_json_file(argv[2]);
  json_node *result = create_result_json_array(coreinfo, &total_tasks);

  for (uint64_t i = 0; i < repeats; ++i) {
    fflush(stdout);
    fflush(stdin);
    fflush(stderr);

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
      if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        fprintf(stderr, "Failed to set scheduler\n");
        fprintf(stderr, "Please run this program as root\n");
        exit(EXIT_FAILURE);
      }
      void *dll = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
      if (dll == NULL) {
        fprintf(stderr, "Unable to find dll %s\n", LIB_NAME);
        fprintf(stderr, "Please set LD_LIBRARY_PATH to the directory "
                        "containing the dll\n");
        exit(EXIT_FAILURE);
      }

      uint64_t core = 0;

      test_args *args = parse_from_json(argv[2], &core);
      load_dll(core, args, LIB_NAME, dll);
      start_test(core, args);
      get_result(core, args, memory);

      free_test_args(core, args);
      dlclose(dll);
      exit(EXIT_SUCCESS);
    } else {
      int status = 0;
      wait(&status);
      if (status != 0) {
        fprintf(stderr, "Child process exited with status 0x%x\n", status);
        exit(EXIT_FAILURE);
      }
      store_results(coreinfo, result, memory);
    }

    // Read the results and reset the cache for the next run
    for (uint64_t j = 0; j < SHARED_MEMORY_SIZE / sizeof(uint64_t); ++j) {
      memory[j] = rand();
    }

    memset(memory, 0, SHARED_MEMORY_SIZE);
  }

  munmap(memory, SHARED_MEMORY_SIZE);
  FILE *output = fopen(argv[3], "w");
  print_json(result, 0, output);
  fprintf(output, "\n");
  fflush(output);
  fclose(output);
  free_json_node(coreinfo);
  free_json_node(result);
  return 0;
}
