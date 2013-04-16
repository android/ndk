LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue36131-flto-c++11
LOCAL_SRC_FILES := issue36131-flto-c++11.cxx
LOCAL_CFLAGS += -g -std=c++11

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
