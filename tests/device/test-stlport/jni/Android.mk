# The source files are actually located under ../unit
LOCAL_PATH := $(dir $(call my-dir))

include $(CLEAR_VARS)
LOCAL_MODULE := test_stlport

unit_path := $(dir $(LOCAL_PATH))/unit
sources := $(wildcard $(unit_path)/*.cpp $(unit_path)/*.c)
sources := $(sources:$(LOCAL_PATH)/%=%)

# Both a C and a C++ source file with the same basename
sources := $(filter-out unit/string_header_test.c,$(sources))

# Exceptions are disabled for now
sources := $(filter-out unit/exception_test.cpp,$(sources))

# This test enters an infinit loop for unknown reasons!
sources := $(filter-out unit/codecvt_test.cpp,$(sources))

LOCAL_SRC_FILES := $(sources)
LOCAL_SRC_FILES += unit/cppunit/test_main.cpp

LOCAL_SHARED_LIBRARIES := libstlport_shared

include $(BUILD_EXECUTABLE)

$(call import-module,cxx-stl/stlport)