LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gabixx_static_rtti
LOCAL_SRC_FILES := test_gabixx_rtti.cpp
LOCAL_STATIC_LIBRARIES := gabi++_static
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_gabixx_shared_rtti
LOCAL_SRC_FILES := test_gabixx_rtti.cpp
LOCAL_SHARED_LIBRARIES := gabi++_shared
include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/gabi++)
