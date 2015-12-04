LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue65705-asm-pc
LOCAL_SRC_FILES := issue65705-asm-pc.c
# Using -static to prevent ndk-build from adding -fPIE even when APP_PIE is set
# to true from the command line. The compiler bug this test is trying to show is
# not present when compiling with -fPIE.
LOCAL_LDFLAGS := -static
include $(BUILD_EXECUTABLE)
