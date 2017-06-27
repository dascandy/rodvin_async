#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "test.h"

size_t strlen(const char *str) {
  size_t n = 0;
  while (str[n]) n++;
  return n;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) { 
    a++; 
    b++; 
  }
  return *a - *b;
}

char *strstr(const char *haystack, const char *needle) {
  if (strlen(needle) > strlen(haystack)) return NULL;
  size_t lim = strlen(haystack) - strlen(needle);
  for (size_t offset = 0; offset < lim; offset++) {
    if (memcmp(haystack+offset, needle, strlen(needle)) == 0)
      return const_cast<char*>(haystack + offset);
  }
  return NULL;
}

char *strdup(const char *str) {
  size_t len = strlen(str);
  char *c = reinterpret_cast<char*>(malloc(len + 1));
  memcpy(c, str, len+1);
  return c;
}

char *strrchr(char *str, char v) {
  char *p = str + strlen(str) - 1;
  while (*p != v && str != p) {
    p--;
  }
  if (*p == v) return p;
  return NULL;
}

char *strchr(char *s, int c) {
  while (*s && *s != c) s++;
  return *s ? s : NULL;
}

char *strcpy(char *dst, const char *src) {
  char *dest = dst;
  while (*src) 
    *dst++ = *src++;
  *dst = 0;
  return dest;
}

void memmove(void *dst, const void *src, size_t count) {
  char *d = reinterpret_cast<char*>(dst);
  const char *s = reinterpret_cast<const char*>(src);
  if (d < s) {
    for (size_t n = 0; n < count; n++) {
      d[n] = s[n];
    }
  } else {
    for (size_t n = count; n > 0; n--) {
      d[n-1] = s[n-1];
    }
  }
}

void memcpy(void *dst, const void *src, size_t count) {
  memmove(dst, src, count);
}

void memset(void *target, uint8_t c, size_t n) {
  uint8_t *t = reinterpret_cast<uint8_t *>(target);
  for (size_t i = 0; i < n; i++) {
    t[i] = c;
  }
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *t1 = (const uint8_t*)s1,
                *t2 = (const uint8_t*)s2;
  for (size_t i = 0; i < n; ++i) {
    if (t1[i] != t2[i]) return t2[i] - t1[i];
  }
  return 0;
}

TEST(memsetWorks) {
  char buffer[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  ASSERT(buffer[1] == 1);
  memset(buffer, 0, 3);
  ASSERT(buffer[2] == 0);
  ASSERT(buffer[0] == 0);
  ASSERT(buffer[3] == 3);
  memset(buffer+2, 5, 7);
  ASSERT(buffer[1] == 0);
  ASSERT(buffer[2] == 5);
  ASSERT(buffer[8] == 5);
  ASSERT(buffer[9] == 9);
}

static void TestString(const rodvin::string& str, const char* ex, bool isLocal) {
  ASSERT(str.isLocal() == isLocal);
  ASSERT(str.size() == strlen(ex));
  ASSERT(strcmp(str.data(), ex) == 0);
}

TEST(simpleStrings) {
  using rodvin::string;
  TestString(string(), "", true);
  TestString(string("some text"), "some text", true);
  TestString(string("text with cutoff", 10), "text with ", true);
  TestString(string(string("some text")), "some text", true);
  TestString(string(string("text with cutoff", 10), 2, 5), "xt wi", true);
  TestString(string("012345678901234567890123456789"), "012345678901234567890123456789", true);
  TestString(string("0123456789012345678901234567890"), "0123456789012345678901234567890", false);
}

TEST(appendToString) {
  using rodvin::string;
  string base("test");
  printf("%s\n", base.data());
  TestString(base + "out", "testout", true);
  base += "outs";
  printf("%s\n", base.data());
  TestString(base, "testouts", true);
  base += " some test input that's long enough to spill";
  printf("%s\n", base.data());
  TestString(base, "testouts some test input that's long enough to spill", false);
}
/*
TEST(spillAndShrink) {
  using rodvin::string;
  string x = "012345678901234567890123456789";
  ASSERT(x.isLocal());
  ASSERT(x.size() == 30);
  x += "1";
  ASSERT(!x.isLocal());
  ASSERT(x.size() == 31);
  x.setSize(30);
  ASSERT(!x.isLocal());
  ASSERT(x.size() == 30);
  x.shrink_to_fit();
  ASSERT(x.isLocal());
  ASSERT(x.size() == 30);
}
*/

