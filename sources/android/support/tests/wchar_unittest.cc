#include <stddef.h>
#include <wchar.h>

#include <minitest/minitest.h>

TEST(wchar, wchar_limits) {
  ASSERT_EQ(4U, sizeof(wchar_t));
  ASSERT_EQ(sizeof(int), sizeof(wint_t));
}

TEST(wchar, wcschr) {
  static const wchar_t kString[] = L"abcda";
  EXPECT_EQ(kString + 0, wcschr(kString, L'a'));
  EXPECT_EQ(kString + 1, wcschr(kString, L'b'));
  EXPECT_EQ(kString + 2, wcschr(kString, L'c'));
  EXPECT_EQ(kString + 3, wcschr(kString, L'd'));
  EXPECT_FALSE(wcschr(kString, L'e'));
  EXPECT_EQ(kString + 5, wcschr(kString, L'\0'));
}

TEST(wchar, wcsrchr) {
  static const wchar_t kString[] = L"abcda";
  EXPECT_EQ(kString + 4, wcsrchr(kString, L'a'));
  EXPECT_EQ(kString + 1, wcsrchr(kString, L'b'));
  EXPECT_EQ(kString + 2, wcsrchr(kString, L'c'));
  EXPECT_EQ(kString + 3, wcsrchr(kString, L'd'));
  EXPECT_FALSE(wcsrchr(kString, L'e'));
  EXPECT_EQ(kString + 5, wcsrchr(kString, L'\0'));
}
