# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.

LOCAL_PATH := $(call my-dir)/../..

llvm_libc++_includes := $(LOCAL_PATH)/include $(LOCAL_PATH)/include/support/android
llvm_libc++_export_includes := $(llvm_libc++_includes)
llvm_libc++_sources := \
	algorithm.cpp \
	bind.cpp \
	chrono.cpp \
	condition_variable.cpp \
	debug.cpp \
	exception.cpp \
	future.cpp \
	hash.cpp \
	ios.cpp \
	iostream.cpp \
	locale.cpp \
	memory.cpp \
	mutex.cpp \
	new.cpp \
	random.cpp \
	regex.cpp \
	stdexcept.cpp \
	string.cpp \
	strstream.cpp \
	system_error.cpp \
	thread.cpp \
	typeinfo.cpp \
	utility.cpp \
	valarray.cpp

llvm_libc++_sources += \
    support/android/locale_support.c \
    support/android/nl_types_support.c \
    support/android/stdlib_support.c \
    support/android/wchar_support.c \
    support/android/wctype_support.c

llvm_libc++_sources := $(llvm_libc++_sources:%=src/%)
llvm_libc++_export_cxxflags := -std=c++11
llvm_libc++_cxxflags := $(llvm_libc++_export_cxxflags) \
    -DGABIXX_LIBCXX=1 -DLIBCXXRT=1

include $(dir $(LOCAL_PATH))../../gabi++/sources.mk
llvm_libc++_includes += $(libgabi++_c_includes)

include $(CLEAR_VARS)
LOCAL_MODULE := llvm_libc++_static
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_SRC_FILES += $(libgabi++_src_files:%=../gabi++/%)
LOCAL_C_INCLUDES := $(llvm_libc++_includes)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := llvm_libc++_shared
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_SRC_FILES += $(libgabi++_src_files:%=../gabi++/%)
LOCAL_C_INCLUDES := $(llvm_libc++_includes)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags) -DGABIXX_LIBCXX=1 -DLIBCXXRT=1
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
include $(BUILD_SHARED_LIBRARY)
