LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_SRC_FILES := main.c
# Use linker flags to list libfoo.so at link time.
# ndk-build should warn about this.
LOCAL_LDLIBS := -lz -L$(NDK_APP_OUT)/local/$(TARGET_ARCH_ABI) -lfoo
include $(BUILD_EXECUTABLE)
