#include <ctype.h>

#include <minitest/minitest.h>

TEST(ctype, isblank) {
  EXPECT_TRUE(isblank(' '));
  EXPECT_TRUE(isblank('\t'));
  EXPECT_FALSE(isblank('\n'));
  EXPECT_FALSE(isblank('\f'));
  EXPECT_FALSE(isblank('\r'));
}

TEST(ctype, isprint) {
  EXPECT_TRUE(isprint('a'));
  EXPECT_TRUE(isprint(' '));
  EXPECT_FALSE(isprint('\t'));
  EXPECT_FALSE(isprint('\n'));
  EXPECT_FALSE(isprint('\f'));
  EXPECT_FALSE(isprint('\r'));
}
