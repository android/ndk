LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := flto
LOCAL_SRC_FILES := flto.c

FLTO_FLAG := -flto
ifneq ($(filter clang%,$(NDK_TOOLCHAIN_VERSION)),)
ifeq ($(TARGET_ARCH_ABI),mips)
# clang does LTO via gold plugin, but gold doesn't support MIPS yet
FLTO_FLAG :=
endif
endif

LOCAL_CFLAGS += $(FLTO_FLAG)
LOCAL_LDFLAGS += $(FLTO_FLAG)
include $(BUILD_EXECUTABLE)

