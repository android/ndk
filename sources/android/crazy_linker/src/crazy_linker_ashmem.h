// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_ASHMEM_H
#define CRAZY_LINKER_ASHMEM_H

#include <unistd.h>

namespace crazy {

// Helper class to hold a scoped ashmem region file descriptor.
class AshmemRegion {
 public:
  AshmemRegion() : fd_(-1) {}

  ~AshmemRegion() { Reset(-1); }

  int Get() const { return fd_; }

  int Release() {
    int ret = fd_;
    fd_ = -1;
    return ret;
  }

  void Reset(int fd) {
    if (fd_ != -1) {
      ::close(fd_);
    }
    fd_ = fd;
  }

  // Try to allocate a new ashmem region of |region_size|
  // (page-aligned) bytes. |region_name| is optional, if not NULL
  // it will be the name of the region (only used for debugging).
  // Returns true on success, false otherwise.
  bool Allocate(size_t region_size, const char* region_name);

  // Change the protection flags of the region. Returns true on success.
  // On failure, check errno for an error code.
  bool SetProtectionFlags(int prot_flags);

 private:
  AshmemRegion(const AshmemRegion& other);
  AshmemRegion& operator=(const AshmemRegion& other);

  int fd_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_ASHMEM_H
