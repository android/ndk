LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := vulkan
LOCAL_SRC_FILES := instance.cpp
LOCAL_CFLAGS := -std=c++11 -DVK_PROTOTYPES
LOCAL_LDLIBS := -lvulkan
LOCAL_SHARED_LIBRARIES := VkLayer_device_limits
LOCAL_SHARED_LIBRARIES += VkLayer_draw_state
LOCAL_SHARED_LIBRARIES += VkLayer_image
LOCAL_SHARED_LIBRARIES += VkLayer_mem_tracker
LOCAL_SHARED_LIBRARIES += VkLayer_object_tracker
LOCAL_SHARED_LIBRARIES += VkLayer_param_checker
LOCAL_SHARED_LIBRARIES += VkLayer_swapchain
LOCAL_SHARED_LIBRARIES += VkLayer_threading
LOCAL_SHARED_LIBRARIES += VkLayer_unique_objects
#LOCAL_STATIC_LIBRARIES := libshaderc_$(APP_STL)
include $(BUILD_EXECUTABLE)

$(call import-module,third_party/vulkan)
