#include "test.h"
#include <stdio.h>

int main() {
  int count = 0, fail = 0;
  runtests(count, fail);
  printf("%d tests of %d failed\n", fail, count);
  return fail > 0;
}


