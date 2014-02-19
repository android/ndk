LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_1_shared
LOCAL_SRC_FILES := test_1.cc
LOCAL_STATIC_LIBRARIES := libc++_static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_1_static
LOCAL_SRC_FILES := test_1.cc
LOCAL_SHARED_LIBRARIES := libc++_shared
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/llvm-libc++)
