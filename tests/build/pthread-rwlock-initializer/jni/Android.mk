LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := test_pthread_rwlock_initializer
LOCAL_CFLAGS := -Wall -Werror
LOCAL_SRC_FILES := test_pthread_rwlock_initializer.c
include $(BUILD_SHARED_LIBRARY)
