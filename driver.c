#include "framework.h"
#include "funcdef.h"
#include "jsonobj.h"
#include "jsonparser.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// May be we can do some json parsing here, so that we do not need to recompile
// every time
const char *helpMsg = "Usage: %s <repeats> <input_json> <output_json>\n";

#define LIB_NAME "libtestfunc.so"

int main(int argc, const char **argv) {
  if (argc < 2 || argc != 4) {
    fprintf(stderr, helpMsg, argv[0]);
    exit(EXIT_FAILURE);
  }
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    fprintf(stderr, helpMsg, argv[0]);
    return 0;
  }

  uint64_t cores = 0;
  uint64_t repeats = atoi(argv[1]);

  void *dll = dlopen(LIB_NAME, RTLD_NOW | RTLD_GLOBAL);
  if (dll == NULL) {
    fprintf(stderr, "Unable to open dll %s\n", LIB_NAME);
    exit(EXIT_FAILURE);
  }

  json_node *results = create_json_node();
  results->type = JSON_ARRAY;
  test_args *args = parse_from_json(argv[2], LIB_NAME, dll, &cores, results);

  for (int64_t i = 0; i < repeats; ++i) {
    start_test(cores, args);
    get_result(cores, args, i);
    dll = reload_dll(LIB_NAME, args);
  }

  free_test_args(cores, args);

  FILE *fp = fopen(argv[3], "w");
  if (fp == NULL) {
    fprintf(stderr, "Failed to open %s\n", argv[3]);
    exit(EXIT_FAILURE);
  }
  print_json(results, 0, fp);

  free_json_node(results);

  if (fp) {
    fclose(fp);
  }

  dlclose(dll);

  return 0;
}
