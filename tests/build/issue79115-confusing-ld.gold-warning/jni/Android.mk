LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue79115-confusing-ld.gold-warning
LOCAL_SRC_FILES := issue79115-confusing-ld.gold-warning.c
LOCAL_DISABLE_NO_EXECUTE := true
LOCAL_LDFLAGS += -Wl,--fatal-warnings
include $(BUILD_EXECUTABLE)
