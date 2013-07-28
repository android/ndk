// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_debug.h"

#include <errno.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <stdarg.h>
#include <stdio.h>

namespace crazy {

#if CRAZY_DEBUG

namespace {

void LogArgs(const char* fmt, va_list args, bool print_error, int error) {
  const size_t buffer_size = 4096;
  char* buffer = reinterpret_cast<char*>(::malloc(buffer_size));
  int ret;

  ret = vsnprintf(buffer, buffer_size, fmt, args);
  if (ret >= static_cast<int>(buffer_size))
    ret = static_cast<int>(buffer_size) - 1;

  if (print_error) {
    strlcat(buffer, ": ", buffer_size);
    strlcat(buffer, strerror(error), buffer_size);
  }

  // First, send to stderr.
  fprintf(stderr, "%.*s", ret, buffer);

#ifdef __ANDROID__
  // Then to the Android log.
  __android_log_write(ANDROID_LOG_INFO, "crazy_linker", buffer);
#endif

  ::free(buffer);
}

}  // namespace

void Log(const char* fmt, ...) {
  int old_errno = errno;
  va_list args;
  va_start(args, fmt);
  LogArgs(fmt, args, false, -1);
  va_end(args);
  errno = old_errno;
}

void LogErrno(const char* fmt, ...) {
  int old_errno = errno;
  va_list args;
  va_start(args, fmt);
  LogArgs(fmt, args, true, old_errno);
  va_end(args);
  errno = old_errno;
}

#endif  // CRAZY_DEBUG

}  // namespace crazy
