LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := foo.cpp
LOCAL_LDLIBS := -latomic
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bar
LOCAL_SRC_FILES := bar.cpp
LOCAL_SHARED_LIBRARIES := libfoo
include $(BUILD_EXECUTABLE)
