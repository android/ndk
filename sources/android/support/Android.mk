LOCAL_PATH := $(call my-dir)

android_support_c_includes := $(LOCAL_PATH)/include

ifneq ($(filter $(NDK_KNOWN_DEVICE_ABI64S),$(TARGET_ARCH_ABI)),)
# 64-bit ABIs
android_support_sources := \
    src/musl-locale/catclose.c \
    src/musl-locale/catgets.c \
    src/musl-locale/catopen.c

else
# 32-bit ABIs

android_support_sources := \
    src/libdl_support.c \
    src/locale_support.c \
    src/math_support.c \
    src/stdlib_support.c \
    src/wchar_support.c \
    src/locale/duplocale.c \
    src/locale/freelocale.c \
    src/locale/localeconv.c \
    src/locale/newlocale.c \
    src/locale/uselocale.c \
    src/stdio/stdio_impl.c \
    src/stdio/strtod.c \
    src/stdio/vfprintf.c \
    src/stdio/vfwprintf.c \
    src/msun/e_log2.c \
    src/msun/e_log2f.c \
    src/msun/s_nan.c \
    src/musl-ctype/iswalnum.c \
    src/musl-ctype/iswalpha.c \
    src/musl-ctype/iswblank.c \
    src/musl-ctype/iswcntrl.c \
    src/musl-ctype/iswctype.c \
    src/musl-ctype/iswdigit.c \
    src/musl-ctype/iswgraph.c \
    src/musl-ctype/iswlower.c \
    src/musl-ctype/iswprint.c \
    src/musl-ctype/iswpunct.c \
    src/musl-ctype/iswspace.c \
    src/musl-ctype/iswupper.c \
    src/musl-ctype/iswxdigit.c \
    src/musl-ctype/towctrans.c \
    src/musl-ctype/wcswidth.c \
    src/musl-ctype/wctrans.c \
    src/musl-ctype/wcwidth.c \
    src/musl-locale/catclose.c \
    src/musl-locale/catgets.c \
    src/musl-locale/catopen.c \
    src/musl-locale/iconv.c \
    src/musl-locale/intl.c \
    src/musl-locale/isalnum_l.c \
    src/musl-locale/isalpha_l.c \
    src/musl-locale/isblank_l.c \
    src/musl-locale/iscntrl_l.c \
    src/musl-locale/isdigit_l.c \
    src/musl-locale/isgraph_l.c \
    src/musl-locale/islower_l.c \
    src/musl-locale/isprint_l.c \
    src/musl-locale/ispunct_l.c \
    src/musl-locale/isspace_l.c \
    src/musl-locale/isupper_l.c \
    src/musl-locale/iswalnum_l.c \
    src/musl-locale/iswalpha_l.c \
    src/musl-locale/iswblank_l.c \
    src/musl-locale/iswcntrl_l.c \
    src/musl-locale/iswctype_l.c \
    src/musl-locale/iswdigit_l.c \
    src/musl-locale/iswgraph_l.c \
    src/musl-locale/iswlower_l.c \
    src/musl-locale/iswprint_l.c \
    src/musl-locale/iswpunct_l.c \
    src/musl-locale/iswspace_l.c \
    src/musl-locale/iswupper_l.c \
    src/musl-locale/iswxdigit_l.c \
    src/musl-locale/isxdigit_l.c \
    src/musl-locale/langinfo.c \
    src/musl-locale/strcasecmp_l.c \
    src/musl-locale/strcoll.c \
    src/musl-locale/strerror_l.c \
    src/musl-locale/strfmon.c \
    src/musl-locale/strftime_l.c \
    src/musl-locale/strncasecmp_l.c \
    src/musl-locale/strxfrm.c \
    src/musl-locale/tolower_l.c \
    src/musl-locale/toupper_l.c \
    src/musl-locale/towctrans_l.c \
    src/musl-locale/towlower_l.c \
    src/musl-locale/towupper_l.c \
    src/musl-locale/wcscoll.c \
    src/musl-locale/wcsxfrm.c \
    src/musl-locale/wctrans_l.c \
    src/musl-locale/wctype_l.c \
    src/musl-math/frexp.c \
    src/musl-math/frexpf.c \
    src/musl-math/frexpl.c \
    src/musl-multibyte/btowc.c \
    src/musl-multibyte/internal.c \
    src/musl-multibyte/mblen.c \
    src/musl-multibyte/mbrlen.c \
    src/musl-multibyte/mbrtowc.c \
    src/musl-multibyte/mbsinit.c \
    src/musl-multibyte/mbsnrtowcs.c \
    src/musl-multibyte/mbsrtowcs.c \
    src/musl-multibyte/mbstowcs.c \
    src/musl-multibyte/mbtowc.c \
    src/musl-multibyte/wcrtomb.c \
    src/musl-multibyte/wcsnrtombs.c \
    src/musl-multibyte/wcsrtombs.c \
    src/musl-multibyte/wcstombs.c \
    src/musl-multibyte/wctob.c \
    src/musl-multibyte/wctomb.c \
    src/musl-stdio/printf.c \
    src/musl-stdio/snprintf.c \
    src/musl-stdio/sprintf.c \
    src/musl-stdio/vprintf.c \
    src/musl-stdio/vsprintf.c \
    src/musl-stdio/swprintf.c \
    src/musl-stdio/vwprintf.c \
    src/musl-stdio/wprintf.c \
    src/wcstox/floatscan.c \
    src/wcstox/intscan.c \
    src/wcstox/shgetc.c \
    src/wcstox/wcstol.c \
    src/wcstox/wcstod.c \

# Replaces broken implementations in x86 libm.so
ifeq (x86,$(TARGET_ARCH_ABI))
android_support_sources += \
    src/musl-math/scalbln.c \
    src/musl-math/scalblnf.c \
    src/musl-math/scalblnl.c \
    src/musl-math/scalbnl.c \

endif

endif  # 64-/32-bit ABIs

# This is only available as a static library for now.
include $(CLEAR_VARS)
LOCAL_MODULE := android_support
LOCAL_SRC_FILES := $(android_support_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes)
LOCAL_CFLAGS += -Drestrict=__restrict__ -ffunction-sections -fdata-sections -fvisibility=hidden
LOCAL_CPPFLAGS += -fvisibility-inlines-hidden

# These Clang warnings are triggered by the Musl sources. The code is fine,
# but we don't want to modify it. TODO(digit): This is potentially dangerous,
# see if there is a way to build the Musl sources in a separate static library
# and have the main one depend on it, or include its object files.
ifneq ($(TARGET_TOOLCHAIN),$(subst clang,,$(TARGET_TOOLCHAIN)))
LOCAL_CFLAGS += \
  -Wno-shift-op-parentheses \
  -Wno-string-plus-int \
  -Wno-dangling-else \
  -Wno-bitwise-op-parentheses
endif

LOCAL_CFLAGS += $(android_support_cflags)
LOCAL_EXPORT_CFLAGS := $(android_support_cflags)
LOCAL_EXPORT_C_INCLUDES := $(android_support_c_includes)
include $(BUILD_STATIC_LIBRARY)

