# This is a trivial shared library that will be imported
# by the main project's binary. Note that it imports
# another library
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libpath1
LOCAL_SRC_FILES := path1.c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)
LOCAL_SHARED_LIBRARIES := libpath2
include $(BUILD_SHARED_LIBRARY)

$(call import-module,path2)
