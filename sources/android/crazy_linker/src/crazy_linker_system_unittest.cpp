// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_system.h"

#include <minitest/minitest.h>
#include <stdlib.h>
#include "crazy_linker_system_mock.h"

namespace crazy {

TEST(System, SingleFile) {
  static const char kPath[] = "/tmp/foo/bar";

  static const char kString[] = "Hello World";
  const size_t kStringLen = sizeof(kString) - 1;

  SystemMock sys;
  sys.AddRegularFile(kPath, kString, kStringLen);

  char buff2[kStringLen + 10];
  FileDescriptor fd(kPath);

  EXPECT_EQ(kStringLen, fd.Read(buff2, sizeof(buff2)));
  buff2[kStringLen] = '\0';
  EXPECT_STREQ(kString, buff2);
}

TEST(System, PathExists) {
  SystemMock sys;
  sys.AddRegularFile("/tmp/foo", "FOO", 3);

  EXPECT_TRUE(PathExists("/tmp/foo"));
}

TEST(System, PathExistsWithBadPath) {
  SystemMock sys;
  EXPECT_FALSE(PathExists("/tmp/foo"));
}

TEST(System, IsSystemLibrary) {
  SystemMock sys;
  static const struct {
    const char* name;
    bool success;
  } kData[] = {
        {"libEGL.so", true},
        {"libGLESv1_CM.so", true},
        {"libGLESv1.so", false},
        {"libGLESv2.so", true},
        {"libOpenMAXAL.so", true},
        {"libOpenSLES.so", true},
        {"libandroid.so", true},
        {"libc.so", true},
        {"libdl.so", true},
        {"libjnigraphics.so", true},
        {"libm.so", true},
        {"libstdc++.so", true},
        {"libstlport.so", false},
        {"libz.so", true},
        {"/system/lib/libc.so", true},
        {"/system/libc.so/libfoo.so", false},
    };
  for (size_t n = 0; n < ARRAY_LEN(kData); ++n) {
    TEST_TEXT << "Checking " << kData[n].name;
    EXPECT_EQ(kData[n].success, IsSystemLibrary(kData[n].name));
  }
}

}  // namespace crazy
