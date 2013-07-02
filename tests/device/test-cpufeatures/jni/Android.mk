LOCAL_PATH := $(call my-dir)

# For unknown arch, we use cpufeatures in libportable.a
# Also note libportable.a will be linked into final binaries automatically
ifneq ($(strip $(filter-out $(NDK_KNOWN_ARCHS),$(TARGET_ARCH))),)
CPUFEATURE_LIBRARY :=
else
CPUFEATURE_LIBRARY := cpufeatures
endif

include $(CLEAR_VARS)
LOCAL_MODULE := test_cpufeatures
LOCAL_SRC_FILES := test_cpufeatures.c
LOCAL_CFLAGS += -I$(NDK_ROOT)/sources/android/cpufeatures
LOCAL_STATIC_LIBRARIES := $(CPUFEATURE_LIBRARY)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpu_1
LOCAL_SRC_FILES := test_android_setCpu_1.c
LOCAL_CFLAGS += -I$(NDK_ROOT)/sources/android/cpufeatures
LOCAL_STATIC_LIBRARIES := $(CPUFEATURE_LIBRARY)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpu_2
LOCAL_SRC_FILES := test_android_setCpu_2.c
LOCAL_CFLAGS += -I$(NDK_ROOT)/sources/android/cpufeatures
LOCAL_STATIC_LIBRARIES := $(CPUFEATURE_LIBRARY)
include $(BUILD_EXECUTABLE)

ifeq ($(TARGET_ARCH),arm)
include $(CLEAR_VARS)
LOCAL_MODULE := test_android_setCpuArm_1
LOCAL_SRC_FILES := test_android_setCpuArm_1.c
LOCAL_CFLAGS += -I$(NDK_ROOT)/sources/android/cpufeatures
LOCAL_STATIC_LIBRARIES := $(CPUFEATURE_LIBRARY)
include $(BUILD_EXECUTABLE)
endif

$(call import-module,android/cpufeatures)
