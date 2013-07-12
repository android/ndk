# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := bitfield

LOCAL_SRC_FILES:= \
    main.c \
    func.c

LOCAL_CFLAGS += -O1

include $(BUILD_EXECUTABLE)
