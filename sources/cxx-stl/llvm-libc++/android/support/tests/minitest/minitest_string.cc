#include "minitest.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

namespace minitest {

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
String& String::operator<<(ParamType v) { \
  char buf[20]; \
  ::snprintf(buf, sizeof(buf), Format, v); \
  (*this) += buf; \
  return *this; \
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

}  // namespace minitest
