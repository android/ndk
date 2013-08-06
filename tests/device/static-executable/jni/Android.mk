LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := static-executable
LOCAL_SRC_FILES := main.cxx
LOCAL_CFLAGS += -fexceptions

# Note that by default ndk-build system doesn't support static executable,
# but option "-static" can instruct gcc to link with proper crt*o files to
# generate static executable.
LOCAL_LDFLAGS += -static

# Be aware that -Wl,--eh-frame-hdr is also needed for "-static" (at least for
# x86 and mips which use eh frames) since crt files are now in C, and the trick
# in the previous *S file to iterate all eh frames and record ranges is gone
LOCAL_LDFLAGS += -Wl,--eh-frame-hdr

include $(BUILD_EXECUTABLE)

