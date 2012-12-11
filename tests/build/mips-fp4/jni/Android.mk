LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := mips-fp4-test1-2
LOCAL_SRC_FILES := mips-fp4-test1-2.c
LOCAL_CFLAGS += -O2 -mips32r2 -save-temps
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := mips-fp4-test3-6
LOCAL_SRC_FILES := mips-fp4-test3-6.c
LOCAL_CFLAGS += -O2 -mips32r2 -ffast-math -save-temps
include $(BUILD_EXECUTABLE)

