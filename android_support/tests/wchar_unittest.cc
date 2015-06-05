#include <limits.h>
#include <stddef.h>
#include <wchar.h>

#include <minitest/minitest.h>

namespace {

const char* to_cstr(const wchar_t* wcs) {
    static char buffer[256];
    size_t n;
    for (n = 0; n + 1U < sizeof(buffer); ++n) {
        wchar_t ch = wcs[n];
        if (!ch)
            break;
        buffer[n] = (ch < 128) ? ch : '@';
    }
    buffer[n] = '\0';
    return buffer;
}

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof(x[0]))

}

TEST(wchar, wchar_limits) {
  ASSERT_EQ(sizeof(__WCHAR_TYPE__), sizeof(wchar_t));
  ASSERT_EQ(sizeof(int), sizeof(wint_t));
#ifdef __arm__
  ASSERT_GT(wchar_t(0), wchar_t(-1));
  ASSERT_EQ(wchar_t(0), WCHAR_MIN);
  ASSERT_EQ(wchar_t(0xffffffff), WCHAR_MAX);
#else
  ASSERT_LT(wchar_t(0), wchar_t(-1));
  ASSERT_EQ(wchar_t(0x80000000), WCHAR_MIN);
  ASSERT_EQ(wchar_t(0x7fffffff), WCHAR_MAX);
#endif
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

TEST(wchar, wcstof) {
    static const struct {
        const wchar_t* input;
        float expected;
        int expected_len;
    } kData[] = {
        { L"123", 123., 3 },
        { L"123#", 123., 3 },
        { L"   123 45", 123., 6 },
        { L"0.2", 0.2, 3 },
        { L"-0.2", -0.2, 4 },
        { L"-3.1415926535", -3.1415926535, 13 },
        { L"+1e+100", 1e100, 7 },
        { L"0x10000.80", 65536.50, 10 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstof(kData[n].input, &end));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstod) {
    static const struct {
        const wchar_t* input;
        double expected;
        int expected_len;
    } kData[] = {
        { L"123", 123., 3 },
        { L"123#", 123., 3 },
        { L"   123 45", 123., 6 },
        { L"0.2", 0.2, 3 },
        { L"-0.2", -0.2, 4 },
        { L"-3.1415926535", -3.1415926535, 13 },
        { L"+1e+100", 1e100, 7 },
        { L"0x10000.80", 65536.50, 10 },
        { L"1.e60", 1e60, 5 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstod(kData[n].input, &end));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstold) {
    static const struct {
        const wchar_t* input;
        long double expected;
        int expected_len;
    } kData[] = {
        { L"123", 123., 3 },
        { L"123#", 123., 3 },
        { L"   123 45", 123., 6 },
        { L"0.2", 0.2L, 3 },
        { L"-0.2", -0.2L, 4 },
        { L"-3.1415926535", -3.1415926535L, 13 },
        { L"+1e+100", 1e100L, 7 },
        { L"0x10000.80", 65536.50L, 10 },
        { L"+1.e+100", 1e100L, 8 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstold(kData[n].input, &end));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstol) {
    static const struct {
        const wchar_t* input;
        int base;
        long expected;
        int expected_len;
    } kData[] = {
        { L"123", 10, 123, 3 },
        { L"123#", 10, 123, 3 },
        { L"01000", 0, 512, 5 },
        { L"   123 45", 0, 123, 6 },
        { L"  -123", 0, -123, 6 },
        { L"0x10000", 0, 65536, 7 },
        { L"12222222222222222222222222222222222", 10, LONG_MAX, 35 },
        { L"-12222222222222222222222222222222222", 10, LONG_MIN, 36 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstol(kData[n].input, &end, kData[n].base));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstoul) {
    static const struct {
        const wchar_t* input;
        int base;
        unsigned long expected;
        int expected_len;
    } kData[] = {
        { L"123", 10, 123, 3 },
        { L"123#", 10, 123, 3 },
        { L"01000", 0, 512, 5 },
        { L"   123 45", 0, 123, 6 },
        { L"  -123", 0, ULONG_MAX - 123 + 1, 6 },
        { L"0x10000", 0, 65536, 7 },
        { L"12222222222222222222222222222222222", 10, ULONG_MAX, 35 },
        { L"-1", 10, ULONG_MAX, 2 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstoul(kData[n].input, &end, kData[n].base));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstoll) {
    static const struct {
        const wchar_t* input;
        int base;
        long long expected;
        int expected_len;
    } kData[] = {
        { L"123", 10, 123, 3 },
        { L"123#", 10, 123, 3 },
        { L"01000", 0, 512, 5 },
        { L"   123 45", 0, 123, 6 },
        { L"  -123", 0, -123, 6 },
        { L"0x10000", 0, 65536, 7 },
        { L"12222222222222222222222222222222222", 10, LLONG_MAX, 35 },
        { L"-12222222222222222222222222222222222", 10, LLONG_MIN, 36 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstoll(kData[n].input, &end, kData[n].base));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}

TEST(wchar, wcstoull) {
    static const struct {
        const wchar_t* input;
        int base;
        unsigned long long expected;
        int expected_len;
    } kData[] = {
        { L"123", 10, 123, 3 },
        { L"123#", 10, 123, 3 },
        { L"01000", 0, 512, 5 },
        { L"   123 45", 0, 123, 6 },
        { L"  -123", 0, ULLONG_MAX - 123 + 1, 6 },
        { L"0x10000", 0, 65536, 7 },
        { L"12222222222222222222222222222222222", 10, ULLONG_MAX, 35 },
        { L"-1", 10, ULLONG_MAX, 2 },
    };
    for (size_t n = 0; n < ARRAY_SIZE(kData); ++n) {
        const char* text = to_cstr(kData[n].input);
        wchar_t* end;
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected, wcstoull(kData[n].input, &end, kData[n].base));
        TEST_TEXT << text;
        EXPECT_EQ(kData[n].expected_len, (int)(end - kData[n].input));
    }
}
