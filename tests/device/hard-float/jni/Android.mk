LOCAL_PATH := $(call my-dir)

# This is the way used to be: For ARM, ndk-build forces "softfp"
# regardless armeabi (default to use -msoft-float) or armeabi-v7a
# (default to use -mfpu=vfpv3-d16)
include $(CLEAR_VARS)
LOCAL_MODULE := float-softfp-abi
LOCAL_SRC_FILES := test-float.c
include $(BUILD_EXECUTABLE)


ifeq ($(filter %armeabi-v7a,$(TARGET_ARCH_ABI)),)

#
# The following two examples are relevant only to armeabi-v7a
#

$(warning Examples hard-float-softfp-abi and hard-float-hard-abi not relevant to ABI $(TARGET_ARCH_ABI))

else

ifeq (,$(filter clang%,$(NDK_TOOLCHAIN_VERSION)))

# This is to compile all user code in -mhard-float (which implies
# -mfloat-abi=hard and overrides -mfloat-abi=softfp previously added
# by ndk-build), but still link system libm.so which use softfp.
# All functions in Android platform headers taking or returning float/double
# has __attribute__((pcs("aapcs"))), so even under the implied -mfloat-abi=hard
# softfp ABI is still observed by compiler for Android native APIs.
#
# You also need to ensure that
# 1. All code and library *must* be recompiled with consistent soft-abi.
#    If your project use ndk-build and import other project, you may want to do
#
#    $NDK/ndk-build -B APP_ABI=armeabi-v7a APP_CFLAGS=-mhard-float APP_LDFLAGS=-Wl,--no-warn-mismatch
#
#    If your project use libraries compiled by others, it's likely those were built with
#    -msoft-abi=softfp or evan -msoft-float (for armeabi).  Recompile them with -mhard-float
#
# 2. Correct headers (eg. #include <math.h>) are always included, and there is no
#    declaration like "extern double sin(double)" w/o proper __attribute__ in
#    your code instead of including math.h, etc.  See the end of sys/cdefs.h
#    the conditions upon which __NDK_FPABI__ and __NDK_FPABI_MATH__ are defined
#
# 3. All JNI functions should have JNICALL which is defined to __NDK_FPABI__ in jni.h.
#
# 4. If you use undocumented APIs which takes/returns float/double, be careful
#    to add __attribute__((pcs("aapcs"))) for arm
#
# Note that "--no-warn-mismatch" is needed for linker (except mclinker which check correctly)
# to suppress linker error about not all functions use VFP register to pass argument, eg.
#
#   .../arm-linux-androideabi/bin/ld: error: ..../test-float.o
#           uses VFP register arguments, output does not
#
include $(CLEAR_VARS)
LOCAL_MODULE := hard-float-softfp-abi
LOCAL_CFLAGS += -mhard-float
LOCAL_SRC_FILES := test-float.c
LOCAL_LDFLAGS := -Wl,--no-warn-mismatch
include $(BUILD_EXECUTABLE)

else

# Clang before 3.4 doesn't allow change of calling convenstion for builtin,
# and produces error message reads:
#
#  a.i:564:6: error: function declared 'aapcs' here was previously declared without calling convention
#  int  sin(double d) __attribute__((pcs("aapcs")));544
#       ^
#  a.i:564:6: note: previous declaration is here
#
# As a result, "-mhard-float" doesn't work properly for now (3.3 and 3.2), unless
# libm_hard is also used.  See example below.
#

$(warning Skip example hard-float-softfp-abi for clang for now)

endif # check clang

# This is to compile all user code in -mhard-float and link a customized
# libm_hard.a which follows -mfloat-abi=hard. _NDK_MATH_NO_SOFTFP=1
# is to turn off __attribute__((pcs("aapcs"))) in math.h for all math
# functions to accept float-abi (implicit "hard" in this case) as specified by user.
#
include $(CLEAR_VARS)
LOCAL_MODULE := hard-float-hard-abi
LOCAL_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1
LOCAL_LDLIBS += -lm_hard
LOCAL_SRC_FILES := test-float.c
LOCAL_LDFLAGS := -Wl,--no-warn-mismatch
include $(BUILD_EXECUTABLE)

endif # check armeabi-v7a
