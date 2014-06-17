#include <math.h>

#include <minitest/minitest.h>

TEST(math, sizeof_long_double) {
#ifdef __ANDROID__
#  ifdef __LP64__
    EXPECT_EQ(16U, sizeof(long double));
#  else
    EXPECT_EQ(8U, sizeof(long double));
#  endif
#endif
}

TEST(math, nexttoward) {
    double x = 2.0;
    EXPECT_EQ(x, nexttoward(x, (long double)x));
    EXPECT_GT(x, nexttoward(x, (long double)(x * 2.)));
    EXPECT_LT(x, nexttoward(x, (long double)(x - 1.0)));
}

TEST(math, nexttowardf) {
    float x = 2.0;
    EXPECT_EQ(x, nexttowardf(x, (long double)x));
    EXPECT_GT(x, nexttowardf(x, (long double)(x * 2.)));
    EXPECT_LT(x, nexttowardf(x, (long double)(x - 1.0)));
}

TEST(math, nexttowardl) {
    long double x = 2.0;
    EXPECT_EQ(x, nexttowardl(x, x));
    EXPECT_GT(x, nexttowardl(x, x * 2.));
    EXPECT_LT(x, nexttowardl(x, x - 1.0));
}

// These functions are not exported on x86 before API level 18!
TEST(math, scalbln) {
    EXPECT_EQ(16., scalbln(2.0, (long int)3));
}

TEST(math, scalblnf) {
    EXPECT_EQ((float)16., scalblnf((float)2.0, (long int)3));
}

TEST(math, scalblnl) {
    EXPECT_EQ((long double)16., scalblnl((long double)2.0, (long int)3));
}
