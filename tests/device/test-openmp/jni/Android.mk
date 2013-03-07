LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := openmp
LOCAL_SRC_FILES := openmp.c
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := openmp2
LOCAL_SRC_FILES := openmp2.c
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := fib
LOCAL_SRC_FILES := fib.c
LOCAL_CFLAGS += -fopenmp
LOCAL_LDFLAGS += -fopenmp
include $(BUILD_EXECUTABLE)

