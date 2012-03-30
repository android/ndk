LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test-build-assembly
ifeq ($(TARGET_ARCH),arm)
    LOCAL_SRC_FILES := assembly1.s assembly2.S
else
    ifeq ($(TARGET_ARCH),x86)
        LOCAL_SRC_FILES := assembly-x86.S
    else
        ifeq ($(TARGET_ARCH),mips)
            LOCAL_SRC_FILES := assembly-mips.S
        endif
    endif
endif
include $(BUILD_SHARED_LIBRARY)
