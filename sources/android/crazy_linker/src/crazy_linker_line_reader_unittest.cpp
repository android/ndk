// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_line_reader.h"

#include <minitest/minitest.h>

#include "crazy_linker_system_mock.h"

namespace crazy {

static const char kFilePath[] = "/tmp/foo.txt";

TEST(LineReader, EmptyConstructor) {
  LineReader reader;
  EXPECT_FALSE(reader.GetNextLine());
}

TEST(LineReader, EmptyFile) {
  SystemMock sys;
  sys.AddRegularFile(kFilePath, "", 0);

  LineReader reader(kFilePath);
  EXPECT_FALSE(reader.GetNextLine());
}

TEST(LineReader, SingleLineFile) {
  SystemMock sys;
  static const char kFile[] = "foo bar\n";
  static const size_t kFileSize = sizeof(kFile) - 1;
  sys.AddRegularFile(kFilePath, kFile, kFileSize);

  LineReader reader(kFilePath);
  EXPECT_TRUE(reader.GetNextLine());
  EXPECT_EQ(kFileSize, reader.length());
  EXPECT_MEMEQ(kFile, kFileSize, reader.line(), reader.length());
  EXPECT_FALSE(reader.GetNextLine());
}

TEST(LineReader, SingleLineFileUnterminated) {
  SystemMock sys;
  static const char kFile[] = "foo bar";
  static const size_t kFileSize = sizeof(kFile) - 1;
  sys.AddRegularFile(kFilePath, kFile, kFileSize);

  LineReader reader(kFilePath);
  EXPECT_TRUE(reader.GetNextLine());
  // The LineReader will add a newline to the last line.
  EXPECT_EQ(kFileSize + 1, reader.length());
  EXPECT_MEMEQ(kFile, kFileSize, reader.line(), reader.length() - 1);
  EXPECT_EQ('\n', reader.line()[reader.length() - 1]);
  EXPECT_FALSE(reader.GetNextLine());
}

TEST(LineReader, MultiLineFile) {
  SystemMock sys;
  static const char kFile[] =
      "This is a multi\n"
      "line text file that to test the crazy::LineReader class implementation\n"
      "And this is a very long text line to check that the class properly "
      "handles them, through the help of dynamic allocation or something. "
      "Yadda yadda yadda yadda. No newline";
  static const size_t kFileSize = sizeof(kFile) - 1;
  sys.AddRegularFile(kFilePath, kFile, kFileSize);

  LineReader reader(kFilePath);

  EXPECT_TRUE(reader.GetNextLine());
  EXPECT_MEMEQ("This is a multi\n", 16, reader.line(), reader.length());

  EXPECT_TRUE(reader.GetNextLine());
  EXPECT_MEMEQ(
      "line text file that to test the crazy::LineReader class "
      "implementation\n",
      88 - 17,
      reader.line(),
      reader.length());

  EXPECT_TRUE(reader.GetNextLine());
  EXPECT_MEMEQ(
      "And this is a very long text line to check that the class properly "
      "handles them, through the help of dynamic allocation or something. "
      "Yadda yadda yadda yadda. No newline\n",
      187 - 17,
      reader.line(),
      reader.length());

  EXPECT_FALSE(reader.GetNextLine());
}

}  // namespace crazy
