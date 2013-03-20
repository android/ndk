LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_unwind_struct
LOCAL_SRC_FILES := main.c
LOCAL_STATIC_LIBRARIES := gabi++_static
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/gabi++)
