LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue36131-flto-c++11
LOCAL_SRC_FILES := issue36131-flto-c++11.cxx
LOCAL_CFLAGS += -g -std=c++11
LOCAL_CFLAGS += -flto
LOCAL_LDFLAGS += -flto

# Clang LTO is only supported with gold. ARM64 still uses bfd by default, so
# make sure this test uses gold when we're using clang.
ifneq ($(filter clang%,$(NDK_TOOLCHAIN_VERSION)),)
LOCAL_CFLAGS += -fuse-ld=gold
LOCAL_LDFLAGS += -fuse-ld=gold
endif

include $(BUILD_EXECUTABLE)
