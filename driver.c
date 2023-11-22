#include <stdio.h>
#include <stdlib.h>
#include "framework.h"
#include "funcdef.h"

#define CORE 4

int main(void) {
  void (*func[])(void *) = {my_loop, my_loop2, my_loop3, my_loop4};
  const char *func_name[] = {"my_loop", "my_loop2", "my_loop3", "my_loop4"};

  test_args *args = create_test_args(CORE);

  for (int i = 0; i < CORE; ++i) {
    add_function(args + i, func_name[i], func[i], NULL);
  }

  // add_function(args, func_name[0], func[0], NULL);

  start_test(CORE, args);

  free_test_args(args);

  return 0;
}