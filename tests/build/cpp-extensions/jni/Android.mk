# Basic test to check that ndk-build recognizes all standard GCC
# C++ file extensions by default.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := test_cpp_extensions
LOCAL_SRC_FILES := \
  foo1.cc \
  foo2.cp \
  foo3.cxx \
  foo4.cpp \
  foo5.C \
  foo6.CPP \
  main.c++

include $(BUILD_EXECUTABLE)
