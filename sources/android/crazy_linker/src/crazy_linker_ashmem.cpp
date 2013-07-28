// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_ashmem.h"

#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/ashmem.h>

#include "crazy_linker_system.h"

namespace crazy {

bool AshmemRegion::Allocate(size_t region_size, const char* region_name) {
  int fd = TEMP_FAILURE_RETRY(open("/dev/ashmem", O_RDWR));
  if (fd < 0)
    return false;

  if (ioctl(fd, ASHMEM_SET_SIZE, region_size) < 0)
    goto ERROR;

  if (region_name) {
    char buf[256];
    strlcpy(buf, region_name, sizeof(buf));
    if (ioctl(fd, ASHMEM_SET_NAME, buf) < 0)
      goto ERROR;
  }

  Reset(fd);
  return true;

ERROR:
  ::close(fd);
  return false;
}

bool AshmemRegion::SetProtectionFlags(int prot) {
  return ioctl(fd_, ASHMEM_SET_PROT_MASK, prot) == 0;
}
}  // namespace crazy
