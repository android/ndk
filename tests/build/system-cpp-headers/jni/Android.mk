LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_system_cpp_headers
LOCAL_SRC_FILES := main.cpp
LOCAL_C_INCLUDES := $(NDK_ROOT)/sources/cxx-stl/system/include
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gabixx_cpp_headers
LOCAL_SRC_FILES := main.cpp
LOCAL_STATIC_LIBRARIES := libgabi++_static
include $(BUILD_EXECUTABLE)

include $(NDK_ROOT)/sources/cxx-stl/gabi++/Android.mk
