// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_SYSTEM_H
#define CRAZY_LINKER_SYSTEM_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "crazy_linker_util.h"  // for String

// System abstraction used by the crazy linker.
// This is used to make unit testing easier without using tons of
// dependency injection in the rest of the code base.
//
// In a nutshell: in a normal build, this will wrap normal open() / read()
// calls. During unit testing, everything is mocked, see
// crazy_linker_system_mock.cpp

namespace crazy {

enum FileOpenMode {
  FILE_OPEN_READ_ONLY = 0,
  FILE_OPEN_READ_WRITE,
  FILE_OPEN_WRITE
};

// Scoping file descriptor class.
class FileDescriptor {
 public:
#ifdef UNIT_TESTS
#define kEmptyFD NULL
  typedef void* HandleType;
#else
#define kEmptyFD (-1)
  typedef int HandleType;
#endif

  FileDescriptor() : fd_(kEmptyFD) {}

  FileDescriptor(const char* path) : fd_(kEmptyFD) { OpenReadOnly(path); }

  ~FileDescriptor() { Close(); }

  bool IsOk() const { return fd_ != kEmptyFD; }
  HandleType Get() const { return fd_; }
  bool OpenReadOnly(const char* path);
  bool OpenReadWrite(const char* path);
  int Read(void* buffer, size_t buffer_size);
  int SeekTo(off_t offset);
  void* Map(void* address,
            size_t length,
            int prot_flags,
            int flags,
            off_t offset);
  void Close();

 private:
  HandleType fd_;
};

// Returns true iff a given file path exists.
bool PathExists(const char* path_name);

// Returns true iff a given path is a regular file (or link to a regular
// file).
bool PathIsFile(const char* path_name);

// Returns the current directory, as a string.
String GetCurrentDirectory();

// Returns the value of a given environment variable.
const char* GetEnv(const char* var_name);

// Returns true iff |lib_name| corresponds to one of the NDK-exposed
// system libraries.
bool IsSystemLibrary(const char* lib_name);

}  // namespace crazy

#endif  // CRAZY_LINKER_SYSTEM_H
