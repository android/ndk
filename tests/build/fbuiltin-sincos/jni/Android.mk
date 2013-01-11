LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fsincos1
LOCAL_SRC_FILES := fbuiltin-sincos.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := fsincos2
LOCAL_SRC_FILES := fbuiltin-sincos.c
LOCAL_CFLAGS += -fbuiltin-sincos -fno-builtin-sincos
include $(BUILD_EXECUTABLE)
