// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_system.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "crazy_linker_util.h"

// Note: unit-testing support files are in crazy_linker_files_mock.cpp

namespace crazy {

#ifndef UNIT_TESTS

bool FileDescriptor::OpenReadOnly(const char* path) {
  Close();
  fd_ = TEMP_FAILURE_RETRY(::open(path, O_RDONLY));
  return (fd_ != -1);
}

bool FileDescriptor::OpenReadWrite(const char* path) {
  Close();
  fd_ = TEMP_FAILURE_RETRY(::open(path, O_RDWR));
  return (fd_ != -1);
}

int FileDescriptor::Read(void* buffer, size_t buffer_size) {
  return TEMP_FAILURE_RETRY(::read(fd_, buffer, buffer_size));
}

int FileDescriptor::SeekTo(off_t offset) {
  return ::lseek(fd_, offset, SEEK_SET);
}

void* FileDescriptor::Map(void* address,
                          size_t length,
                          int prot,
                          int flags,
                          off_t offset) {
  return ::mmap(address, length, prot, flags, fd_, offset);
}

void FileDescriptor::Close() {
  if (fd_ != -1) {
    int old_errno = errno;
    TEMP_FAILURE_RETRY(close(fd_));
    errno = old_errno;
    fd_ = -1;
  }
}

const char* GetEnv(const char* var_name) { return ::getenv(var_name); }

String GetCurrentDirectory() {
  String result;
  size_t capacity = 128;
  for (;;) {
    result.Resize(capacity);
    if (getcwd(&result[0], capacity))
      break;
    capacity *= 2;
  }
  return result;
}

bool PathExists(const char* path) {
  struct stat st;
  if (TEMP_FAILURE_RETRY(stat(path, &st)) < 0)
    return false;

  return S_ISREG(st.st_mode) || S_ISDIR(st.st_mode);
}

bool PathIsFile(const char* path) {
  struct stat st;
  if (TEMP_FAILURE_RETRY(stat(path, &st)) < 0)
    return false;

  return S_ISREG(st.st_mode);
}

#endif  // !UNIT_TESTS

// Returns true iff |lib_name| corresponds to one of the NDK-exposed
// system libraries.
bool IsSystemLibrary(const char* lib_name) {
  static const char* const kSystemLibs[] = {
      "libandroid.so",   "libc.so",         "libdl.so",     "libjnigraphics.so",
      "liblog.so",       "libm.so",         "libstdc++.so", "libz.so",
      "libEGL.so",       "libGLESv1_CM.so", "libGLESv2.so", "libGLESv3.so",
      "libOpenMAXAL.so", "libOpenSLES.so", };
  const size_t kSize = sizeof(kSystemLibs) / sizeof(kSystemLibs[0]);
  const char* base_name = ::strrchr(lib_name, '/');
  if (!base_name)
    base_name = lib_name;
  else
    base_name += 1;

  for (size_t n = 0; n < kSize; ++n) {
    if (!strcmp(kSystemLibs[n], base_name))
      return true;
  }
  return false;
}

}  // namespace crazy
