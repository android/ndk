# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.

LOCAL_PATH := $(call my-dir)

llvm_libc++_includes := $(LOCAL_PATH)/libcxx/include
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
	optional.cpp \
	random.cpp \
	regex.cpp \
	shared_mutex.cpp \
	stdexcept.cpp \
	string.cpp \
	strstream.cpp \
	system_error.cpp \
	thread.cpp \
	typeinfo.cpp \
	utility.cpp \
	valarray.cpp \
	support/android/locale_android.cpp

llvm_libc++_sources := $(llvm_libc++_sources:%=libcxx/src/%)

# For now, this library can only be used to build C++11 binaries.
llvm_libc++_export_cxxflags := -std=c++11

llvm_libc++_cxxflags := $(llvm_libc++_export_cxxflags)

# Building this library with -fno-rtti is not supported, though using it
# without RTTI is ok.
#
llvm_libc++_cxxflags += -fno-rtti

# Gabi++ emulates libcxxabi when building libcxx.
#
llvm_libc++_cxxflags += -DLIBCXXABI=1

# Find the GAbi++ sources to include them here.
# The voodoo below is to allow building libc++ out of the NDK source
# tree. This can make it easier to experiment / update / debug it.
#
libgabi++_sources_dir := $(strip $(wildcard $(LOCAL_PATH)/../gabi++))
ifdef libgabi++_sources_dir
  libgabi++_sources_prefix := ../gabi++
else
  libgabi++_sources_dir := $(strip $(wildcard $(NDK_ROOT)/sources/cxx-stl/gabi++))
  ifndef libgabi++_sources_dir
    $(error Can't find GAbi++ sources directory!!)
  endif
  libgabi++_sources_prefix := $(libgabi++_sources_dir)
endif

include $(libgabi++_sources_dir)/sources.mk
llvm_libc++_sources += $(addprefix $(libgabi++_sources_prefix:%/=%)/,$(libgabi++_src_files))
llvm_libc++_includes += $(libgabi++_c_includes)
llvm_libc++_export_includes += $(libgabi++_c_includes)

include $(CLEAR_VARS)
LOCAL_MODULE := libc++_static
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_C_INCLUDES := $(llvm_libc++_includes)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
LOCAL_STATIC_LIBRARIES := android_support compiler_rt_static
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libc++_shared
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_C_INCLUDES := $(llvm_libc++_includes)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
LOCAL_STATIC_LIBRARIES := android_support
LOCAL_SHARED_LIBRARIES := compiler_rt_shared
include $(BUILD_SHARED_LIBRARY)

$(call import-module, android/support)
$(call import-module, android/compiler-rt)
