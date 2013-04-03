LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue53646-stlport-stl_confix_h
LOCAL_SRC_FILES := issue53646-stlport-stl_confix_h.cpp
LOCAL_CFLAGS += -D__EDG_VERSION__=244
include $(BUILD_EXECUTABLE)
