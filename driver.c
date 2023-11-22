#include <stdio.h>
#include <stdlib.h>
#include "framework.h"
#include "funcdef.h"

#define TOTAL_CORE 4

int main(void) {
  void (*func[])(void *) = {my_loop, my_loop2, my_loop3, my_loop4};
  const char *func_name[] = {"my_loop", "my_loop2", "my_loop3", "my_loop4"};

  // create args
  test_args args[TOTAL_CORE][4];

  for (int i = 0; i < TOTAL_CORE; ++i) {
    create_arg(&args[i][0], func_name[i], func[i], NULL);
  }

  start_test(TOTAL_CORE, (test_args **)args);

  return 0;
}