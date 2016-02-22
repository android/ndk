# Copyright 2015 The Android Open Source Project
# Copyright (C) 2015 Valve Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(abspath $(call my-dir))
MY_PATH := $(LOCAL_PATH)
SRC_DIR := $(LOCAL_PATH)

include $(CLEAR_VARS)
LOCAL_MODULE := layer_utils
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_config.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_extension_utils.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_utils.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_draw_state
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/draw_state.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_debug_marker_table.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers \
                    $(SRC_DIR)/glslang/SPIRV
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_mem_tracker
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/mem_tracker.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_device_limits
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/device_limits.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_debug_marker_table.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_image
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/image.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_param_checker
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/param_checker.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_debug_marker_table.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_object_tracker
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/object_tracker.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_threading
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/threading.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
                    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_unique_objects
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/unique_objects.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_safe_struct.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
		    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := VkLayer_swapchain
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/swapchain.cpp
LOCAL_SRC_FILES += $(SRC_DIR)/validation-layers/vk_layer_table.cpp
LOCAL_C_INCLUDES += $(SRC_DIR)/include \
		    $(SRC_DIR)/validation-layers
LOCAL_STATIC_LIBRARIES += layer_utils
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)

