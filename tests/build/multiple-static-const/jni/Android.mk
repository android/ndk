LOCAL_PATH := $(my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := test_multiple_static_const
LOCAL_SRC_FILES := main.cpp foo.cpp

include $(BUILD_EXECUTABLE)

