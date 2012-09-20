LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue28598_liba
LOCAL_SRC_FILES := liba.cpp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := issue28598_main
LOCAL_SRC_FILES := main.cpp
LOCAL_SHARED_LIBRARIES := issue28598_liba
include $(BUILD_EXECUTABLE)
