LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.c

# LOCAL_LDLIBS are always ignored for static libraries.
# This should generate a warning when ndk-build is run!
LOCAL_LDLIBS := -llog

include $(BUILD_STATIC_LIBRARY)
