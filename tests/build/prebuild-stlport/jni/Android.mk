# See README to understand what this project does.
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := build_stlport_static
LOCAL_SRC_FILES := build_stlport.cpp
LOCAL_STATIC_LIBRARIES := stlport_static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := build_stlport_shared
LOCAL_SRC_FILES := build_stlport.cpp
LOCAL_SHARED_LIBRARIES := stlport_shared
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/stlport)
