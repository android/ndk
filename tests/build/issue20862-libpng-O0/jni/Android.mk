LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_MODULE:= png

LOCAL_CFLAGS := -I$(LOCAL_PATH) -O0 -g

LOCAL_SRC_FILES := $(notdir $(filter-out %/example.c, $(wildcard $(LOCAL_PATH)/*.c)))

LOCAL_LDLIBS :=

include $(BUILD_SHARED_LIBRARY)
