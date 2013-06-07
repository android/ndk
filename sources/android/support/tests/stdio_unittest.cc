#include <stdio.h>
#include <wchar.h>

#include <minitest/minitest.h>

TEST(stdio,snprintf) {
  char char_buff[32];
  size_t char_buff_len = sizeof(char_buff) / sizeof(char);
  EXPECT_EQ(2, snprintf(char_buff, char_buff_len, "ab"));
  EXPECT_EQ(5, snprintf(char_buff, char_buff_len, "%s", "abcde"));
  static const char kString[] = "Hello, World";
  EXPECT_EQ(12, snprintf(char_buff, char_buff_len, "%s", kString));
  EXPECT_EQ(12, snprintf(char_buff, 13, "%s", kString));

  EXPECT_EQ(12, snprintf(char_buff, 12, "%s", kString));
  EXPECT_EQ(L'\0', char_buff[11]);
  EXPECT_EQ(12, snprintf(char_buff, 1, "%s", kString));
  EXPECT_EQ(L'\0', char_buff[0]);
}

TEST(stdio,swprintf) {
  wchar_t wide_buff[32];
  size_t wide_buff_len = sizeof(wide_buff) / sizeof(wchar_t);
  EXPECT_EQ(2, swprintf(wide_buff, wide_buff_len, L"ab"));
  EXPECT_EQ(5, swprintf(wide_buff, wide_buff_len, L"%s", "abcde"));
  static const wchar_t kWideString[] = L"Hello\uff41 World";
  EXPECT_EQ(12, swprintf(wide_buff, wide_buff_len, L"%ls", kWideString));
  EXPECT_EQ(12, swprintf(wide_buff, 13, L"%ls", kWideString));

  // Unlike snprintf(), swprintf() returns -1 in case of truncation
  // and doesn't necessarily zero-terminate the output!
  EXPECT_EQ(-1, swprintf(wide_buff, 12, L"%ls", kWideString));
  EXPECT_EQ(-1, swprintf(wide_buff, 1, L"%ls", kWideString));
}

