LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue22165_throwable
LOCAL_SRC_FILES := throwable.cpp
LOCAL_CPP_FEATURES := exceptions
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := issue22165_main
LOCAL_SRC_FILES := main.cpp
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES := issue22165_throwable
include $(BUILD_EXECUTABLE)
