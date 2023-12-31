#include "funcdef.h"

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