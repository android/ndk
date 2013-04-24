LOCAL_PATH := $(call my-dir)

# Define a single static library in this Android.mk, it should be
# compiled
include $(CLEAR_VARS)
LOCAL_MODULE := foo
LOCAL_SRC_FILES := foo.c
include $(BUILD_STATIC_LIBRARY)

# Import another module, which is not used by the one declared here.
# The build.sh will check that it is _not_ compiled.
#
$(call import-module,android/cpufeatures)