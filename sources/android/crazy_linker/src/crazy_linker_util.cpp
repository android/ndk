// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_util.h"

#include <stdio.h>

namespace crazy {

// Return the base name from a file path. Important: this is a pointer
// into the original string.
// static
const char* GetBaseNamePtr(const char* path) {
  const char* p = strrchr(path, '/');
  if (!p)
    return path;
  else
    return p + 1;
}

// static
const char String::kEmpty[] = "";

String::String() { Init(); }

String::String(const String& other) {
  Init();
  Assign(other.ptr_, other.size_);
}

String::String(const char* str) {
  Init();
  Assign(str, strlen(str));
}

String::String(char ch) {
  Init();
  Assign(&ch, 1);
}

String::~String() {
  if (ptr_ != const_cast<char*>(kEmpty)) {
    free(ptr_);
    ptr_ = const_cast<char*>(kEmpty);
  }
}

String::String(const char* str, size_t len) {
  Init();
  Assign(str, len);
}

void String::Assign(const char* str, size_t len) {
  Resize(len);
  if (len > 0) {
    memcpy(ptr_, str, len);
    ptr_[len] = '\0';
    size_ = len;
  }
}

void String::Append(const char* str, size_t len) {
  if (len > 0) {
    size_t old_size = size_;
    Resize(size_ + len);
    memcpy(ptr_ + old_size, str, len);
  }
}

void String::Resize(size_t new_size) {
  if (new_size > capacity_) {
    size_t new_capacity = capacity_;
    while (new_capacity < new_size) {
      new_capacity += (new_capacity >> 1) + 16;
    }
    Reserve(new_capacity);
  }

  if (new_size > size_)
    memset(ptr_ + size_, '\0', new_size - size_);

  size_ = new_size;
  if (ptr_ != kEmpty)
    ptr_[size_] = '\0';
}

void String::Reserve(size_t new_capacity) {
  char* old_ptr = (ptr_ == const_cast<char*>(kEmpty)) ? NULL : ptr_;
  // Always allocate one more byte for the trailing \0
  ptr_ = reinterpret_cast<char*>(realloc(old_ptr, new_capacity + 1));
  ptr_[new_capacity] = '\0';
  capacity_ = new_capacity;

  if (size_ > new_capacity)
    size_ = new_capacity;
}

#if 0
String Format(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  String result(FormatArgs(fmt, args));
  va_end(args);
  return result;
}

String FormatArgs(const char* fmt, va_list args) {
  String result;
  for (;;) {
    va_list args2;
    va_copy(args2, args);
    int ret = vsnprintf(&result[0], result.capacity(), fmt, args2);
    va_end(args2);
    if (static_cast<size_t>(ret) <= result.capacity())
      break;

    result.Resize(static_cast<size_t>(ret));
  }
  return result;
}
#endif

}  // namespace crazy
