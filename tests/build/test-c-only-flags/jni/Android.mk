LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test-c-only-flags
LOCAL_SRC_FILES := test-c-only-flags.cpp
LOCAL_CFLAGS += -Werror

# LOCAL_CFLAGS is passed for C++ compilation as well.  This is by design
# to save developers from the trouble of specifying flags twice for both
# LOCAL_CFLAGS and LOCAL_CPPFLAGS.  Unfortunately for flags like -std=gnu99
# is harmful to g++ (warning) and clang++ (error)
LOCAL_CONLYFLAGS += -std=gnu99

include $(BUILD_EXECUTABLE)
