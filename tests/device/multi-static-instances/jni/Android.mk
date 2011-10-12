LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_multi_static_instances
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := libstdc++
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_multi_static_instances_stlport
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := stlport_static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_multi_static_instances_gnustl
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := gnustl_static
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/gnu-libstdc++)
$(call import-module,cxx-stl/stlport)
$(call import-module,cxx-stl/system)