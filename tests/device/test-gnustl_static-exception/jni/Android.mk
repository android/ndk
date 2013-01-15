LOCAL_PATH := $(call my-dir)

# Test -1: Throw in exec, catch in exec covered elsewhere

# Test 0: Throw in lib, catch in lib

include $(CLEAR_VARS)
LOCAL_MODULE := test0_foo
LOCAL_SRC_FILES := test0_foo.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Lib_catching_lib_throwing
LOCAL_SRC_FILES := test0_main.cpp
LOCAL_STATIC_LIBRARIES := test0_foo
include $(BUILD_EXECUTABLE)

# Test 1: Throw in lib, catch in exec
include $(CLEAR_VARS)
LOCAL_MODULE := test1_foo
LOCAL_SRC_FILES := test1_foo.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := exec_catching_lib_throwing
LOCAL_SRC_FILES := test1_main.cpp
LOCAL_STATIC_LIBRARIES := test1_foo
include $(BUILD_EXECUTABLE)

# Test 2: Throw in exec, catch in lib
# Note: Symbol lookup from lib at exec fixed in bionic after android-14

include $(CLEAR_VARS)
LOCAL_MODULE := test2_foo
LOCAL_SRC_FILES := test2_foo.cpp
# This is required to rely on dynamic loader to resolve foo defined in exec
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := exec_throwing_lib_catching
LOCAL_SRC_FILES := test2_main.cpp
LOCAL_STATIC_LIBRARIES := test2_foo
include $(BUILD_EXECUTABLE)

#
# 3 libs cases
#

# Test 3: Throw in lib2, catch in exec

include $(CLEAR_VARS)
LOCAL_MODULE := test3_bar
LOCAL_SRC_FILES := test3_bar.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test3_foo
LOCAL_SRC_FILES := test3_foo.cpp
LOCAL_STATIC_LIBRARIES := test3_bar
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := exec_catching_lib2_throwing
LOCAL_SRC_FILES := test3_main.cpp
LOCAL_STATIC_LIBRARIES := test3_foo test3_bar
include $(BUILD_EXECUTABLE)

# Test 4: Throw in lib2, catch in lib1

include $(CLEAR_VARS)
LOCAL_MODULE := test4_bar
LOCAL_SRC_FILES := test4_bar.cpp
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test4_foo
LOCAL_SRC_FILES := test4_foo.cpp
LOCAL_STATIC_LIBRARIES := test4_bar
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := Lib1_catching_lib2_throwing
LOCAL_SRC_FILES := test4_main.cpp
LOCAL_STATIC_LIBRARIES := test4_foo test4_bar
include $(BUILD_EXECUTABLE)

