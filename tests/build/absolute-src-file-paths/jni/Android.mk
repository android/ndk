LOCAL_PATH := $(call my-dir)

ifndef FOO_PATH
# We may reach here running this test from run-tests.sh
# when it Android.mk is included in make -f $NDK/build/core/build-local.mk DUMP_*
# to determin ABIs.  In this case FOO_PATH isn't set and doesn't matter.
# For normal build, empty FOO_PATH causes make to find /foo.c and /main.c
# and fail to build.
$(info FOO_PATH should be defined before including this file!)
endif

ifeq (,$(call host-path-is-absolute,$(FOO_PATH)))
$(info FOO_PATH should be defined to an absolute path!)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libfoo
LOCAL_SRC_FILES := $(FOO_PATH)/foo.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_absolute_src_file_path
LOCAL_SRC_FILES := $(FOO_PATH)/main.c
LOCAL_SHARED_LIBRARIES := libfoo
include $(BUILD_EXECUTABLE)
