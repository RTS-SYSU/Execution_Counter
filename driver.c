#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <linux/perf_event.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include "framework.h"
#include "jsonobj.h"
#include "jsonparser.h"

const char *helpMsg =
    "Usage: %s <repeats> <input_json> <output_json> [-e perf_event_id]\n";

const char *help_perf_event =
    "\tCurrent support perf_event: \n"
    "\t\t0: cache-misses\n"
    "\t\t1: cache-references\n"
    "\t\t2: L1-icache-load-misses\n"
    "\t\t3: L1-dcache-load-misses\n"
    "\t\t4: L1-icache-loads\n"
    "\t\t5: L1-dcache-loads\n"
    "\t\t6: L1-dcache-store-misses\n"
    "\t\t7: bus-cycles\n"
    "\t\t8: L1-icache-prefetch-misses\n"
    "\t\t9: L1-dcache-prefetch-misses\n"
    "\t\t10: L1-icache-prefetches\n"
    "\t\t11: L1-dcache-prefetches\n"
    "\t\t12: instructions\n";

// #define LIB_NAME "libtestfunc.so"

#define SHARED_MEMORY_SIZE 4096

#define PRINT_HELP                   \
  fprintf(stderr, helpMsg, argv[0]); \
  fprintf(stderr, "%s", help_perf_event)

#define PRINT_HELP_EXIT(EXITCODE) \
  PRINT_HELP;                     \
  exit(EXITCODE)

int main(int argc, const char **argv) {
  if (argc != 2 && argc != 4 && argc != 6) {
    PRINT_HELP_EXIT(EXIT_FAILURE);
  }
  if (argc == 2 &&
      (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    PRINT_HELP;
    return 0;
  }

  int perf_id = -1;
  const char *perf_item = "ticks";
  if (argc == 6) {
    if (strcmp(argv[4], "-e") != 0) {
      fprintf(stderr, "Unsupported option: %s\n", argv[4]);
      PRINT_HELP_EXIT(EXIT_FAILURE);
    }
    perf_id = atoi(argv[5]);
    switch (perf_id) {
      case 0:
        perf_item = "cache-misses";
        break;
      case 1:
        perf_item = "cache-references";
        break;
      case 2:
        perf_item = "L1-icache-load-misses";
        break;
      case 3:
        perf_item = "L1-dcache-load-misses";
        break;
      case 4:
        perf_item = "L1-icache-loads";
        break;
      case 5:
        perf_item = "L1-dcache-loads";
        break;
      case 6:
        perf_item = "L1-dcache-store-misses";
        break;
      case 7:
        perf_item = "bus-cycles";
        break;
      case 8:
        perf_item = "L1-icache-prefetch-misses";
        break;
      case 9:
        perf_item = "L1-dcache-prefetch-misses";
        break;
      case 10:
        perf_item = "L1-icache-prefetches";
        break;
      case 11:
        perf_item = "L1-dcache-prefetches";
        break;
      case 12:
        perf_item = "instructions";
        break;
      default:
        fprintf(stderr, "Unsupported perf_event_id: %d\n", perf_id);
        PRINT_HELP_EXIT(EXIT_FAILURE);
    }
  }

  uint64_t repeats = atoi(argv[1]);

  // Create a 4K shared memory using mmap
  // This is used to store the results
  uint64_t *memory =
      (uint64_t *)mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS, -1, 0);

  memset(memory, 0, SHARED_MEMORY_SIZE);
  uint64_t total_tasks = 0;
  json_node *coreinfo_ptr = parse_json_file(argv[2]);
  json_node *coreinfo = json_get(coreinfo_ptr, "entries");
  json_node *result = create_result_json_array(coreinfo, &total_tasks);

  // Make the main thread only run on CPU0
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
    fprintf(stderr, "Failed to set affinity for main thread.\n");
    exit(EXIT_FAILURE);
  }

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
      // void *dll = dlopen(LIB_NAME, RTLD_NOW | RTLD_LOCAL);
      // if (dll == NULL) {
      //   fprintf(stderr, "Unable to find dll %s\n", LIB_NAME);
      //   fprintf(stderr, "Please set LD_LIBRARY_PATH to the directory "
      //                   "containing the dll\n");
      //   exit(EXIT_FAILURE);
      // }

      uint64_t core = 0;

      // printf("perf_item: %s\n", perf_item);
      // printf("perf_id: %d\n", perf_id);

      test_args *args = parse_from_json(argv[2], &core, perf_id);
      // load_dll(core, args, LIB_NAME, dll);
      start_test(core, args);
      get_result(core, args, memory);

      free_test_args(core, args);
      // dlclose(dll);
      exit(EXIT_SUCCESS);
    } else {
      int status = 0;
      wait(&status);
      if (status != 0) {
        fprintf(stderr, "Child process exited with status 0x%x\n", status);
        exit(EXIT_FAILURE);
      }
      store_results(coreinfo, result, memory, perf_item);
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
