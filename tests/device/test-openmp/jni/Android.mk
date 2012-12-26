LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := openmp
LOCAL_SRC_FILES := openmp.c
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
include $(BUILD_EXECUTABLE)

