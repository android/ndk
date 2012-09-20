LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_test_copy_vector_into_a_set
LOCAL_SRC_FILES := copy_vector_into_a_set.cpp
LOCAL_CFLAGS += -DBUG=1
include $(BUILD_EXECUTABLE)
