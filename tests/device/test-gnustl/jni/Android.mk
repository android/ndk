# The source files are actually located under ../unit
LOCAL_PATH := $(dir $(call my-dir))

include $(CLEAR_VARS)
LOCAL_MODULE := test_gnustl

unit_path := $(LOCAL_PATH)/unit
sources := $(wildcard $(unit_path)/*.cpp $(unit_path)/*.c)
sources := $(sources:$(unit_path)/%=%)

# Both a C and a C++ source file with the same basename
sources := $(filter-out string_header_test.c,$(sources))

# Exceptions are disabled for now
sources := $(filter-out exception_test.cpp,$(sources))

# This test enters an infinit loop for unknown reasons!
sources := $(filter-out codecvt_test.cpp,$(sources))

LOCAL_SRC_FILES := $(sources:%=unit/%)
LOCAL_SRC_FILES += unit/cppunit/test_main.cpp

include $(BUILD_EXECUTABLE)
