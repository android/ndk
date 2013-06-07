#ifndef MINITEST_MINITEST_H
#define MINITEST_MINITEST_H

// Minimal unit testing framework for the Android support library.
// Unit tests must be written in C++, but no C++ STL implementation
// is required.

#include <stdio.h>
#include <string.h>

namespace minitest {

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

// A helper function that can be used to generate formatted output
// as a temporary string. Use like printf(), but returns a new String
// object. Useful to stream into TEST_CONTEXT() objects, as in:
//
//   TEST_CONTEXT << "Using "
//                << minitest::Format("%08d", bytes)
//                << " bytes";
//
String Format(const char* format, ...);

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

private:
  Result result_;
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
  typedef minitest::AddConst<__typeof__(expr)>::type \
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
      minitest::String minitest_str; \
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

#define EXPECT_STREQ(expected, expression) \
  do { \
    const char* minitest_expected = (expected); \
    const char* minitest_actual = (expression); \
    if (minitest_actual == NULL) { \
      printf("EXPECT_STREQ:%s:%d: expression '%s' is NULL!\n", \
             __FILE__, __LINE__, #expression); \
    } else if (strcmp(minitest_expected, minitest_actual) != 0) { \
      printf("EXPECT_STREQ:%s:%d: with expression '%s'\n", \
             __FILE__, __LINE__, #expression); \
      printf("actual   : %s\n", minitest_actual); \
      printf("expected : %s\n", minitest_expected); \
      minitest_testcase->Failure(); \
    } \
  } while (0)

#define EXPECT_STRNEQ(expected, expression) \
  do { \
    const char* minitest_expected = (expected); \
    const char* minitest_actual = (expression); \
    if (minitest_actual == NULL) { \
      printf("EXPECT_STRNEQ:%s:%d: expression '%s' is NULL!\n", \
             __FILE__, __LINE__, #expression); \
    } else if (strcmp(minitest_expected, minitest_actual) != 0) { \
      printf("EXPECT_STRNEQ:%s:%d: with expression '%s'\n", \
             __FILE__, __LINE__, #expression); \
      printf("actual   : %s\n", minitest_actual); \
      printf("expected : %s\n", minitest_expected); \
      minitest_testcase->Failure(); \
    } \
  } while (0)

#define ASSERT_TRUE(expression) \
  do { \
    if (!(expression)) { \
      printf("ASSERT_TRUE:%s:%d: expression '%s' return 'false', expected 'true'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcaseFatalFailure(); \
      return; \
    } \
  } while (0)

#define ASSERT_FALSE(expression) \
  do { \
    if (!!(expression)) { \
      printf("ASSERT_FALSE:%s:%d: expression '%s' return 'true', expected 'false'\n", \
             __FILE__, __LINE__, #expression); \
      minitest_testcaseFatalFailure(); \
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
  do { \
    const char* minitest_expected = (expected); \
    const char* minitest_actual = (expression); \
    if (minitest_actual == NULL) { \
      printf("ASSERT_STREQ:%s:%d: expression '%s' is NULL!\n", \
             __FILE__, __LINE__, #expression); \
    } else if (strcmp(minitest_expected, minitest_actual) != 0) { \
      printf("ASSERT_STREQ:%s:%d: with expression '%s'\n", \
             __FILE__, __LINE__, #expression); \
      printf("actual   : %s\n", minitest_actual); \
      printf("expected : %s\n", minitest_expected); \
      minitest_testcaseFatalFailure(); \
      return; \
    } \
  } while (0)

#define ASSERT_STRNEQ(expected, expression) \
  do { \
    const char* minitest_expected = (expected); \
    const char* minitest_actual = (expression); \
    if (minitest_actual == NULL) { \
      printf("ASSERT_STRNEQ:%s:%d: expression '%s' is NULL!\n", \
             __FILE__, __LINE__, #expression); \
    } else if (strcmp(minitest_expected, minitest_actual) != 0) { \
      printf("ASSERT_STRNEQ:%s:%d: with expression '%s'\n", \
             __FILE__, __LINE__, #expression); \
      printf("actual   : %s\n", minitest_actual); \
      printf("expected : %s\n", minitest_expected); \
      minitest_testcaseFatalFailure(); \
      return; \
    } \
  } while (0)

}  // namespace minitest

#endif  // MINITEST_MINITEST_H
