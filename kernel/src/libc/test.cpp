#include "test.h"
#include <stdio.h>

#ifdef LOCAL_BUILD
Test* Test::firstTest = nullptr;

void register_test(Test* aTest) {
  aTest->nextTest = Test::firstTest;
  Test::firstTest = aTest;
}

bool runtests(int& testCount, int& testFails) {
  for (Test* test = Test::firstTest; test; test = test->nextTest) {
    try {
      testCount++;
      test->Runtest();
    } catch (const TestError& aError) {
      testFails++;
      printf("Test %s failed: \n%s\n", test->name, aError.what());
    }
  }
  return testFails == 0;
}
#else
bool runtests(int&, int&) {
  return true;
}
#endif

