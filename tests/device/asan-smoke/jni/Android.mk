LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := asan_smoke
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := asan_oob_test.cc
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -fsanitize=address -fno-omit-frame-pointer
LOCAL_LDFLAGS := -fsanitize=address
LOCAL_STATIC_LIBRARIES := googletest_main
include $(BUILD_EXECUTABLE)

$(call import-module,third_party/googletest)
