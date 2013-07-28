// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_thread.h"

#include <minitest/minitest.h>

namespace crazy {

TEST(Thread, GetThreadData) {
  ThreadData* data = GetThreadData();
  TEST_TEXT << "Checking first GetThreadData() call";
  EXPECT_TRUE(data);
  EXPECT_EQ(data, GetThreadData());
  EXPECT_EQ(data, GetThreadDataFast());
}

TEST(Thread, GetErrorEmpty) {
  ThreadData* data = GetThreadData();
  const char* error = data->GetError();
  EXPECT_TRUE(error);
  EXPECT_STREQ("", error);
}

TEST(Thread, SetError) {
  ThreadData* data = GetThreadData();
  data->SetError("Hello");
  data->SetError("World");
  EXPECT_STREQ("World", data->GetError());
}

TEST(Thread, SetErrorNull) {
  ThreadData* data = GetThreadData();
  data->SetError("Hello");
  data->SetError(NULL);
  EXPECT_STREQ("", data->GetError());
}

TEST(Thread, GetError) {
  ThreadData* data = GetThreadData();
  data->SetError("Hello");

  const char* error = data->GetError();
  EXPECT_STREQ("Hello", error);

  error = data->GetError();
  EXPECT_STREQ("Hello", error);
}

TEST(Thread, SwapErrorBuffers) {
  ThreadData* data = GetThreadData();
  data->SetError("Hello");
  EXPECT_STREQ("Hello", data->GetError());

  data->SwapErrorBuffers();
  EXPECT_STREQ("", data->GetError());

  data->SetError("World");
  EXPECT_STREQ("World", data->GetError());

  data->SwapErrorBuffers();
  EXPECT_STREQ("", data->GetError());
}

TEST(Thread, AppendErrorTwice) {
  ThreadData* data = GetThreadData();
  data->SetError(NULL);
  data->AppendError("Hello");
  EXPECT_STREQ("Hello", data->GetError());

  data->AppendError(" World");
  EXPECT_STREQ("Hello World", data->GetError());
}

TEST(Thread, AppendErrorFull) {
  const size_t kMaxCount = 1000;
  ThreadData* data = GetThreadData();
  data->SetError(NULL);

  for (size_t n = 0; n < kMaxCount; ++n)
    data->AppendError("0123456789");

  const char* error = data->GetError();
  size_t error_len = strlen(error);

  EXPECT_GT(0, error_len);
  EXPECT_LT(kMaxCount * 10, error_len);

  for (size_t n = 0; n < error_len; ++n) {
    TEST_TEXT << "Checking error[" << n << "]";
    EXPECT_EQ('0' + (n % 10), error[n]);
  }
}

TEST(Thread, AppendErrorNull) {
  ThreadData* data = GetThreadData();
  data->SetError("Hello");
  data->AppendError(NULL);
  data->AppendError(" World");
  EXPECT_STREQ("Hello World", data->GetError());
}

TEST(Thread, SetLinkerErrorString) {
  ThreadData* data = GetThreadData();

  SetLinkerErrorString("Hello World");
  EXPECT_STREQ("Hello World", data->GetError());

  SetLinkerErrorString(NULL);
  EXPECT_STREQ("", data->GetError());
}

TEST(Thread, SetLinkerError) {
  ThreadData* data = GetThreadData();

  SetLinkerError("%s %s!", "Hi", "Captain");
  EXPECT_STREQ("Hi Captain!", data->GetError());

  SetLinkerError("Woosh");
  EXPECT_STREQ("Woosh", data->GetError());
}

}  // namespace crazy
