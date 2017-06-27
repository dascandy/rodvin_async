#ifndef TEST_H
#define TEST_H

// TODO: change to enable only if you want tests, rather than this.
#ifdef LOCAL_BUILD
#include <exception>
struct Test;
void register_test(Test* aTest);
struct Test {
  Test(const char* name) : name(name) { register_test(this); }
  virtual void Runtest() = 0;
  static Test* firstTest;
  Test* nextTest;
  const char* name;
};
bool runtests(int& testCount, int& testFails);

#define TEST_F(X, F) struct TestCase##X : public F { \
  TestCase##X() : F(#X) {} \
  void Runtest() override; \
} _inst##X; \
void TestCase##X::Runtest()

#define TEST(X) TEST_F(X, Test)

struct TestError : public std::exception {
  TestError(const char* testError)
  : error(testError)
  {}
  ~TestError() throw() {}
  const char* what() const throw() {
    return error;
  }
  const char* error;
};

#define ASSERT(X) do { if (!(X)) { throw TestError("Test failed: " #X " was not true"); } } while(0)

#else 
struct Test {
  Test(const char*) {}
  virtual void Runtest() {}
};
bool runtests(int&, int&);
#define TEST_F(X, F) void unused_##X() 
#define TEST(X) void unused_##X()
#define ASSERT(X) (void)(X)

#endif

#endif


