LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := foo
LOCAL_SRC_FILES := foo.c foo2.c
include $(BUILD_STATIC_LIBRARY)

# The GNU linker will strip un-needed object files from executables even whe
# --whole-archive is used. However, it won't do it for shared libraries, so
# create an intermediate one, called 'bar' that includes 'foo' as a whole
# static library.
include $(CLEAR_VARS)
LOCAL_MODULE := bar
LOCAL_WHOLE_STATIC_LIBRARIES := foo
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_whole_static_libs
LOCAL_SRC_FILES := main.c
LOCAL_LDLIBS := -ldl
include $(BUILD_EXECUTABLE)
