LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := issue54465-invalid-asm-operand-out-of-range
LOCAL_SRC_FILES := issue54465-invalid-asm-operand-out-of-range.cpp
LOCAL_CPPFLAGS += -O0
include $(BUILD_SHARED_LIBRARY)