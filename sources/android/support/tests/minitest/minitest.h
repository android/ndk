// Copyright (C) 2013 The Android Open Source Project
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// // Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// // Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef MINITEST_MINITEST_H
#define MINITEST_MINITEST_H

// Minitest is a minimalistic unit testing framework designed specifically
// for the Android support library.
//
// - Unit tests must be written in C++, but no C++ runtime implementation
//   is either required or _supported_. This is by design.
//
// _ Inspired by GoogleTest, you can use the TEST macro to define a new
//   test case easily, and it can contain EXPECT_XX|ASSERT_XX statements.
//
//   For example:
//
//      TEST(stdio, strlen) {
//        EXPECT_EQ(3, strlen("foo"));
//        EXPECT_EQ(1, srlen("a"));
//      }
//
//   However, there are important differences / limitations:
//
//   o You cannot stream strings into an except or assert statement.
//     Minitest provides a TEST_TEXT macro to do that instead.
//
//     In other words, replace:
//
//       EXPECT_EQ(expected, expression) << "When checking 'foo'";
//
//     With:
//       TEST_TEXT << "When checking 'foo'";
//       EXPECT_EQ(expected, expression);
//
//    The context text is only printed on failure.
//
//   o TEST_F() is not supported (for now).
//
//   o EXPECT/ASSERT statements only work inside a TESET() function, not
//     in a function called by it.
//
//   o No test death detection.
//
// - You can use minitest::Format() to stream formatted output into
//   a TEST_TEXT context, as in:
//
//      for (size_t n = 0; n < kMaxCount; n++) {
//        TEST_TEXT << "Checking string : "
//                  << minitest;:Format("%z/%z", n+1, kMaxCount);
//        EXPECT_EQ(kExpected[n], Foo(kString[n]));
//      }
//
#include <stdio.h>
#include <string.h>

namespace minitest {

namespace internal {

// AddConst<T>::type adds a 'const' qualifier to type T.
// Examples:
//   int -> const int
//   char* -> const char* const
//   const char* -> const char* const
//   char* const -> const char* const
template <typename T>
struct AddConst {
  typedef const T type;
};
template <typename T>
struct AddConst<const T> {
  typedef const T type;
};
template <typename T>
struct AddConst<T*> {
  typedef const T* const type;
};
template <typename T>
struct AddConst<const T*> {
  typedef const T* const type;
};

// String class used to accumulate error messages.
// Very similar to std::string but also supports streaming into it
// for easier formatted output, as in:
//
//    String str;
//    int x = 42;
//    str << "x is '" << x << "'\n";
//
// You can also use minitest::Format() as in:
//
//    str << minitest::Format("Hex value %08x\n", x);
//
class String {
public:
  String() : str_(NULL), size_(0), capacity_(0) {}
  String(const char* str, size_t len);

  explicit String(const char* str) {
    String(str, ::strlen(str));
  }

  String(const String& other) {
    String(other.str_, other.size_);
  }

  String& operator=(const String& other) {
    (*this) += other;
    return *this;
  }

  char& operator[](size_t index) {
    return str_[index];
  }

  ~String() { Clear(); }

  const char* c_str() const { return str_; }
  const char* data() const { return str_; }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  String& operator+=(const String& other);
  String& operator+=(const char* str);
  String& operator+=(char ch);

  String operator+(const String& other) const {
    String result(*this);
    result += other;
    return result;
  }

  String operator+(const char* str) const {
    String result(*this);
    result += str;
    return result;
  }

  // Basic formatting operators.
  String& operator<<(const String& other);
  String& operator<<(const char* str);

#define MINITEST_OPERATOR_LL_(ParamType) \
    String& operator<<(ParamType v)

  MINITEST_OPERATOR_LL_(bool);
  MINITEST_OPERATOR_LL_(char);
  MINITEST_OPERATOR_LL_(signed char);
  MINITEST_OPERATOR_LL_(short);
  MINITEST_OPERATOR_LL_(int);
  MINITEST_OPERATOR_LL_(long);
  MINITEST_OPERATOR_LL_(long long);
  MINITEST_OPERATOR_LL_(unsigned char);
  MINITEST_OPERATOR_LL_(unsigned short);
  MINITEST_OPERATOR_LL_(unsigned int);
  MINITEST_OPERATOR_LL_(unsigned long);
  MINITEST_OPERATOR_LL_(unsigned long long);
  MINITEST_OPERATOR_LL_(float);
  MINITEST_OPERATOR_LL_(double);
  MINITEST_OPERATOR_LL_(const void*);

#undef MINITEST_OPERATOR_LL_

  void Clear();
  void Resize(size_t new_size);

private:
  void Reserve(size_t new_capacity);
  char* str_;
  size_t size_;
  size_t capacity_;
};

}  // namespace internal

// A helper function that can be used to generate formatted output
// as a temporary string. Use like printf(), but returns a new String
// object. Useful to stream into TEST_TEXT() objects, as in:
//
//   TEST_TEXT << "Using "
//                << minitest::Format("%08d", bytes)
//                << " bytes";
//
internal::String Format(const char* format, ...);

class TestCase {
public:
  enum Result {
    PASS = 0,
    FAIL,
    FATAL
  };

  TestCase() : result_(PASS) {}
  ~TestCase() {}

  void Failure();
  void FatalFailure();

  Result result() { return result_; }

  internal::String& GetText();

private:
  Result result_;
  internal::String text_;
};

// Type of a function defined through the TEST(<test>,<case>) macro.
typedef void (TestFunction)(TestCase* testcase);

// Used internally to register new test functions.
void RegisterTest(const char* test_name,
                  const char* case_name,
                  TestFunction* test_function);

#define MINITEST_TEST_FUNCTION(testname, casename) \
    MINITEST_TEST_FUNCTION_(testname, casename)

#define MINITEST_TEST_FUNCTION_(testname, casename) \
    minitest_##testname##_##casename

#define TEST(testname, casename) \
  static void MINITEST_TEST_FUNCTION(testname, casename)(minitest::TestCase*); \
  static void __attribute__((constructor)) \
  RegisterMiniTest##testname##_##casename() { \
    minitest::RegisterTest(#testname, #casename, \
                           &MINITEST_TEST_FUNCTION(testname, casename)); \
  } \
  void MINITEST_TEST_FUNCTION(testname, casename)(\
      minitest::TestCase* minitest_testcase)

// Use this macro to add context text before an EXPECT or ASSERT statement
// For example:
//
//    for (size_t n = 0; n < MAX; ++n) {
//      TEST_TEXT << "When checking " << kStrings[n];
//      EXPECT_STREQ(kExpected[n], kStrings[n]);
//    }
//
// The text will only be printed in case of failure in the next
// EXPECT/ASSERT statement.
//
#define TEST_TEXT minitest_testcase->GetText()

// EXPECT_TRUE() must evaluate to something that supports the << operator
// to receive debugging strings only in case of failure.
#define EXPECT_TRUE(expression) \
  do { \
    if (!(expression)) { \
      printf("EXPECT_TRUE:%s:%d: expression '%s' returned 'false', expected 'true'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcase->Failure(); \
    } \
  } while (0)

#define EXPECT_FALSE(expression) \
  do { \
    if (!!(expression)) { \
      printf("EXPECT_FALSE:%s:%d: expression '%s' returned 'true', expected 'false'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcase->Failure(); \
    } \
  } while (0)

#define MINITEST_DEFINE_LOCAL_EXPR_(varname, expr) \
  typedef minitest::internal::AddConst<__typeof__(expr)>::type \
    varname##Type; \
  const varname##Type varname = (expr);

#define MINITEST_EXPECT_ASSERT_BINOP_(opname, op, expected, expression, is_assert) \
  do { \
    MINITEST_DEFINE_LOCAL_EXPR_(minitest_expected, expected); \
    MINITEST_DEFINE_LOCAL_EXPR_(minitest_actual, expression); \
    if (!(minitest_actual op minitest_expected)) { \
      printf("%s" #opname ":%s:%d: with expression '%s'\n", \
             is_assert ? "ASSERT_" : "EXPECT_", __FILE__, __LINE__, \
             #expression); \
      minitest::internal::String minitest_str; \
      minitest_str << minitest_actual; \
      printf("actual   : %s\n", minitest_str.c_str()); \
      minitest_str.Clear(); \
      minitest_str << minitest_expected; \
      printf("expected : %s\n", minitest_str.c_str()); \
      if (is_assert) { \
        minitest_testcase->FatalFailure(); \
        return; \
      } \
      minitest_testcase->Failure(); \
    } \
  } while (0)

#define MINITEST_EXPECT_BINOP_(opname, op, expected, expression) \
    MINITEST_EXPECT_ASSERT_BINOP_(opname, op, expected, expression, false)

#define EXPECT_EQ(expected, expression)  \
    MINITEST_EXPECT_BINOP_(EQ, ==, expected, expression)

#define EXPECT_NE(expected, expression)  \
    MINITEST_EXPECT_BINOP_(NE, !=, expected, expression)

#define EXPECT_LE(expected, expression)  \
    MINITEST_EXPECT_BINOP_(LE, <=, expected, expression)

#define EXPECT_LT(expected, expression)  \
    MINITEST_EXPECT_BINOP_(LT, <, expected, expression)

#define EXPECT_GE(expected, expression)  \
    MINITEST_EXPECT_BINOP_(GE, >=, expected, expression)

#define EXPECT_GT(expected, expression)  \
    MINITEST_EXPECT_BINOP_(GT, >, expected, expression)

#define MINITEST_EXPECT_ASSERT_STR_(expected, expression, is_eq, is_assert) \
  do { \
    const char* minitest_prefix = is_assert ? "ASSERT_STR" : "EXPECT_STR"; \
    const char* minitest_suffix = is_eq ? "EQ" : "NEQ"; \
    const char* minitest_expected = (expected); \
    const char* minitest_actual = (expression); \
    if (minitest_actual == NULL) { \
      printf("%s%s:%s:%d: expression '%s' is NULL!\n", \
             minitest_prefix, minitest_suffix, \
             __FILE__, __LINE__, #expression); \
    } else { \
      bool minitest_eq = !strcmp(minitest_expected, minitest_actual); \
      if (minitest_eq != is_eq) { \
        printf("%s%s:%s:%d: with expression '%s'\n", \
               minitest_prefix, minitest_suffix, \
               __FILE__, __LINE__, #expression); \
        printf("actual   : %s\n", minitest_actual); \
        printf("expected : %s\n", minitest_expected); \
        minitest_testcase->Failure(); \
      } \
    } \
  } while (0)

#define EXPECT_STREQ(expected, expression) \
  MINITEST_EXPECT_ASSERT_STR_(expected, expression, true, false)

#define EXPECT_STRNEQ(expected, expression) \
  MINITEST_EXPECT_ASSERT_STR_(expected, expression, false, true)

#define ASSERT_TRUE(expression) \
  do { \
    if (!(expression)) { \
      printf("ASSERT_TRUE:%s:%d: expression '%s' return 'false', expected 'true'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcase->FatalFailure(); \
      return; \
    } \
  } while (0)

#define ASSERT_FALSE(expression) \
  do { \
    if (!!(expression)) { \
      printf("ASSERT_FALSE:%s:%d: expression '%s' return 'true', expected 'false'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcase->FatalFailure(); \
      return; \
    } \
  } while (0)

#define MINITEST_ASSERT_BINOP_(opname, op, expected, expression) \
    MINITEST_EXPECT_ASSERT_BINOP_(opname, op, expected, expression, true)

#define ASSERT_EQ(expected, expression)  \
    MINITEST_ASSERT_BINOP_(EQ, ==, expected, expression)

#define ASSERT_NE(expected, expression)  \
    MINITEST_ASSERT_BINOP_(NE, !=, expected, expression)

#define ASSERT_LE(expected, expression)  \
    MINITEST_ASSERT_BINOP_(LE, <=, expected, expression)

#define ASSERT_LT(expected, expression)  \
    MINITEST_ASSERT_BINOP_(LT, <, expected, expression)

#define ASSERT_GE(expected, expression)  \
    MINITEST_ASSERT_BINOP_(GE, >=, expected, expression)

#define ASSERT_GT(expected, expression)  \
    MINITEST_ASSERT_BINOP_(GT, >, expected, expression)

#define ASSERT_STREQ(expected, expression) \
    MINITEST_EXPECT_ASSERT_STR_(expected, expression, true, true)

#define ASSERT_STRNEQ(expected, expression) \
    MINITEST_EXPECT_ASSERT_STR_(expected, expression, false, true)

}  // namespace minitest

#endif  // MINITEST_MINITEST_H
