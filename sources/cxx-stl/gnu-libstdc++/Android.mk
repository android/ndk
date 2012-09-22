LOCAL_PATH := $(call my-dir)

# Compute the compiler flags to export by the module.
# This is controlled by the APP_GNUSTL_FORCE_CPP_FEATURES variable.
# See docs/APPLICATION-MK.html for all details.
#
gnustl_exported_cppflags := $(strip \
  $(if $(filter exceptions,$(APP_GNUSTL_FORCE_CPP_FEATURES)),-fexceptions)\
  $(if $(filter rtti,$(APP_GNUSTL_FORCE_CPP_FEATURES)),-frtti))

# Include path to export
gnustl_exported_c_includes := $(LOCAL_PATH)/$(TOOLCHAIN_VERSION)/include $(LOCAL_PATH)/$(TOOLCHAIN_VERSION)/libs/$(TARGET_ARCH_ABI)/include

# NDK_APP_USE_BITCODE doesn't support gnustl for now
ifeq ($(NDK_APP_USE_BITCODE),true)
$(call __ndk_warning, "gnu-libstdc++ runtime for bitcode library is not supported yet.")

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_static
LOCAL_SRC_FILES := 4.6/libs/$(TARGET_ARCH_ABI)/libgnustl_static.a
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/4.6/include $(LOCAL_PATH)/4.6/libs/$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_shared
LOCAL_SRC_FILES := 4.6/libs/$(TARGET_ARCH_ABI)/libgnustl_shared.so
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/4.6/include $(LOCAL_PATH)/4.6/libs/$(TARGET_ARCH_ABI)/include
LOCAL_EXPORT_LDLIBS := -lgnustl_shared
include $(PREBUILT_SHARED_LIBRARY)

else

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_static
LOCAL_SRC_FILES := $(TOOLCHAIN_VERSION)/libs/$(TARGET_ARCH_ABI)/libgnustl_static.a
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(gnustl_exported_c_includes)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gnustl_shared
LOCAL_SRC_FILES := $(TOOLCHAIN_VERSION)/libs/$(TARGET_ARCH_ABI)/libgnustl_shared.so
LOCAL_EXPORT_CPPFLAGS := $(gnustl_exported_cppflags)
LOCAL_EXPORT_C_INCLUDES := $(gnustl_exported_c_includes)
LOCAL_EXPORT_LDLIBS := $(call host-path,$(LOCAL_PATH)/$(TOOLCHAIN_VERSION)/libs/$(TARGET_ARCH_ABI)/libsupc++.a)
include $(PREBUILT_SHARED_LIBRARY)

endif
