LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := main.c
LOCAL_SHARED_LIBRARIES := libpath1
include $(BUILD_SHARED_LIBRARY)

$(call import-module,path1)

