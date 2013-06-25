LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := sdktest
LOCAL_SRC_FILES := main.cpp 
LOCAL_C_INCLUDES += jni/boost
LOCAL_CFLAGS += -fexceptions -frtti -DCALL_X

#LOCAL_CFLAGS += -O2

include $(BUILD_EXECUTABLE)

