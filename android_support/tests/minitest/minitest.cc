#include "minitest.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

namespace {

struct TestInfo {
  const char* test_name;
  const char* case_name;
  minitest::TestFunction* test_function;
  TestInfo* next;
};

TestInfo* g_test_infos;
TestInfo** g_test_infos_tail;

}  // namespace

namespace minitest {

namespace internal {

String::String(const char* str, size_t len) {
  Resize(len);
  ::memcpy(str_, str, len);
  size_ = len;
}

String& String::operator+=(const String& other) {
  size_t old_size = size_;
  Resize(old_size + other.size_);
  ::memcpy(str_ + old_size, other.str_, other.size_);
  return *this;
}

String& String::operator+=(const char* str) {
  size_t len = ::strlen(str);
  size_t old_size = size_;
  Resize(old_size + len);
  ::memcpy(str_ + old_size, str, len);
  return *this;
}

String& String::operator+=(char ch) {
  Resize(size_ + 1);
  str_[size_ - 1] = ch;
  return *this;
}

String& String::operator<<(const String& other) {
  (*this) += other;
  return *this;
}

String& String::operator<<(const char* str) {
  (*this) += str;
  return *this;
}

String& String::operator<<(char ch) {
  (*this) += ch;
  return *this;
}

String& String::operator<<(bool v) {
  (*this) += (v ? "true" : "false");
  return *this;
}

#define MINITEST_STRING_OPERATOR_LL_(ParamType, Format) \
  String& String::operator<<(ParamType v) {             \
    char buf[20];                                       \
    ::snprintf(buf, sizeof(buf), Format, v);            \
    (*this) += buf;                                     \
    return *this;                                       \
  }

MINITEST_STRING_OPERATOR_LL_(signed char, "%hhd")
MINITEST_STRING_OPERATOR_LL_(unsigned char, "%hhu")
MINITEST_STRING_OPERATOR_LL_(short, "%hd")
MINITEST_STRING_OPERATOR_LL_(unsigned short, "%hu");
MINITEST_STRING_OPERATOR_LL_(int, "%d")
MINITEST_STRING_OPERATOR_LL_(unsigned, "%u")
MINITEST_STRING_OPERATOR_LL_(long, "%ld")
MINITEST_STRING_OPERATOR_LL_(unsigned long, "%lu")
MINITEST_STRING_OPERATOR_LL_(long long, "%lld")
MINITEST_STRING_OPERATOR_LL_(unsigned long long, "%llu")
MINITEST_STRING_OPERATOR_LL_(float, "%f")
MINITEST_STRING_OPERATOR_LL_(double, "%f")
MINITEST_STRING_OPERATOR_LL_(long double, "%Lf")
MINITEST_STRING_OPERATOR_LL_(const void*, "%p")

#undef MINITEST_STRING_OPERATOR_LL_

void String::Clear() {
  ::free(str_);
  str_ = NULL;
  size_ = 0;
  capacity_ = 0;
}

void String::Resize(size_t new_size) {
  if (new_size > capacity_) {
    size_t new_capacity = capacity_;
    while (new_capacity < new_size)
      new_capacity += (new_capacity >> 1) + 8;

    Reserve(new_capacity);
  }
  str_[new_size] = '\0';
  size_ = new_size;
}

void String::Reserve(size_t new_capacity) {
  str_ = reinterpret_cast<char*>(::realloc(str_, new_capacity + 1));
  if (new_capacity > capacity_)
    ::memset(str_ + capacity_, '\0', new_capacity - capacity_);
  capacity_ = new_capacity;
}

}  // namespace internal

internal::String Format(const char* format, ...) {
  internal::String result;
  va_list args, args2;
  va_start(args, format);
  // Note: Resize(n) allocates at least n+1 bytes.
  result.Resize(100);
  int len;
  for (;;) {
    va_copy(args2, args);
    len = vsnprintf(&result[0], result.size(), format, args2);
    va_end(args2);
    // On Windows, snprintf() returns -1 on truncation. On other
    // platforms, it returns the size of the string, without truncation.
    if (len >= 0 && static_cast<size_t>(len) <= result.size())
      break;
    result.Resize(result.size() * 2);
  }
  va_end(args);
  return result;
}

void TestCase::Failure() {
  if (result_ == PASS)
    result_ = FAIL;
  if (!text_.empty())
    printf("%s\n", text_.c_str());
}

void TestCase::FatalFailure() {
  result_ = FATAL;
  if (!text_.empty())
    printf("%s\n", text_.c_str());
}

internal::String& TestCase::GetText() {
  text_.Clear();
  return text_;
}

void RegisterTest(const char* test_name,
                  const char* case_name,
                  TestFunction* test_function) {
  if (g_test_infos_tail == NULL)
    g_test_infos_tail = &g_test_infos;

  TestInfo* info = reinterpret_cast<TestInfo*>(::malloc(sizeof(*info)));

  info->test_name = test_name;
  info->case_name = case_name;
  info->test_function = test_function;

  *g_test_infos_tail = info;
  g_test_infos_tail = &info->next;
}

}  // namespace minitest

int main(void) {
  printf("--- TESTS STARTING ---\n");
  TestInfo* info = g_test_infos;
  unsigned num_failures = 0;
  unsigned num_tests = 0;
  for (; info != NULL; info = info->next) {
    minitest::TestCase testcase;
    printf("[ RUNNING   ] %s.%s\n", info->test_name, info->case_name);
    num_tests += 1;
    info->test_function(&testcase);
    const char* status;
    switch (testcase.result()) {
      case minitest::TestCase::PASS:
        status = "OK";
        break;
      case minitest::TestCase::FAIL:
      case minitest::TestCase::FATAL:
        status = "FAIL";
        num_failures += 1;
        break;
    }
    printf("[ %9s ] %s.%s\n", status, info->test_name, info->case_name);
  }
  printf("--- TESTS COMPLETED ---\n");
  printf("tests completed: %d\n", num_tests);
  printf("tests passed: %d\n", num_tests - num_failures);
  printf("tests failed: %d\n", num_failures);

  return (num_failures > 0);
}
