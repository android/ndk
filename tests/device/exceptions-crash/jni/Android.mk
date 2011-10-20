LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo_static
LOCAL_SRC_FILES := foo.cpp
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES := gnustl_static
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_exception_crash_static
LOCAL_SRC_FILES := main.cpp
LOCAL_SHARED_LIBRARIES := libfoo_static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo_shared
LOCAL_SRC_FILES := foo.cpp
LOCAL_CPP_FEATURES := exceptions
LOCAL_SHARED_LIBRARIES := gnustl_shared
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_exception_crash_shared
LOCAL_SRC_FILES := main.cpp
LOCAL_SHARED_LIBRARIES := libfoo_shared
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/gnu-libstdc++)