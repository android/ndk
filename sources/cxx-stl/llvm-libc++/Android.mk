# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.

LOCAL_PATH := $(call my-dir)

# Normally, we distribute the NDK with prebuilt binaries of libc++
# in $LOCAL_PATH/libs/<abi>/. However,
#

LIBCXX_FORCE_REBUILD := $(strip $(LIBCXX_FORCE_REBUILD))
ifndef LIBCXX_FORCE_REBUILD
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/libc++_static$(TARGET_LIB_EXTENSION))))
    $(call __ndk_info,WARNING: Rebuilding libc++ libraries from sources!)
    $(call __ndk_info,You might want to use $$NDK/build/tools/build-cxx-stl.sh --stl=libc++)
    $(call __ndk_info,in order to build prebuilt versions to speed up your builds!)
    LIBCXX_FORCE_REBUILD := true
  endif
endif

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

ifeq (,$(filter clang%,$(NDK_TOOLCHAIN_VERSION)))
# Add -fno-strict-aliasing because __list_imp::_end_ breaks TBAA rules by declaring
# simply as __list_node_base then casted to __list_node derived from that.  See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61571 for details
llvm_libc++_export_cxxflags += -fno-strict-aliasing
endif

llvm_libc++_cxxflags := $(llvm_libc++_export_cxxflags)
llvm_libc++_cflags :=

libcxxabi_sources_dir := $(strip $(wildcard $(LOCAL_PATH)/../llvm-libc++abi))
ifdef libcxxabi_sources_dir
  libcxxabi_sources_prefix := ../llvm-libc++abi
else
  libcxxabi_sources_dir := $(strip $(wildcard $(NDK_ROOT)/sources/cxx-stl/llvm-libc++abi))
  ifndef libcxxabi_sources_dir
    $(error Can't find libcxxabi sources directory!!)
  endif
  libcxxabi_sources_prefix := $(libcxxabi_sources_dir)
endif

include $(libcxxabi_sources_dir)/sources.mk

ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
# for armeabi*, use llvm libunwind
llvm_libc++_sources += $(addprefix $(libcxxabi_sources_prefix:%/=%)/,$(libcxxabi_src_files))
llvm_libc++_cxxflags += -DLIBCXXABI_USE_LLVM_UNWINDER=1 -D__STDC_FORMAT_MACROS
else
llvm_libc++_sources += $(addprefix $(libcxxabi_sources_prefix:%/=%)/,$(libcxxabi_src_base_files))
llvm_libc++_cxxflags += -DLIBCXXABI_USE_LLVM_UNWINDER=0
endif

llvm_libc++_includes += $(libcxxabi_c_includes)
llvm_libc++_export_includes += $(libcxxabi_c_includes)
llvm_libc++_cflags += -D__STDC_FORMAT_MACROS

ifeq (clang3.5,$(NDK_TOOLCHAIN_VERSION))
# Workaround an issue of integrated-as (default in clang3.5) where it fails to compile
# llvm-libc++abi/libcxxabi/src/Unwind/UnwindRegistersRestore.S
llvm_libc++_cflags += -no-integrated-as
endif

ifneq ($(LIBCXX_FORCE_REBUILD),true)

$(call ndk_log,Using prebuilt libc++ libraries)

android_support_c_includes := $(LOCAL_PATH)/../../android/support/include

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
# For armeabi*, choose thumb mode unless LOCAL_ARM_MODE := arm
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
ifneq (arm,$(LOCAL_ARM_MODE))
ifneq (arm,$(TARGET_ARM_MODE))
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/thumb/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
endif
endif
endif
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes) $(android_support_c_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
# For armeabi*, choose thumb mode unless LOCAL_ARM_MODE := arm
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
ifneq (arm,$(LOCAL_ARM_MODE))
ifneq (arm,$(TARGET_ARM_MODE))
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/thumb/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
endif
endif
endif
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes) $(android_support_c_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
include $(PREBUILT_SHARED_LIBRARY)

else
# LIBCXX_FORCE_REBUILD == true

$(call ndk_log,Rebuilding libc++ libraries from sources)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes) $(llvm_libc++_includes)
LOCAL_CFLAGS := $(llvm_libc++_cflags)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
LOCAL_STATIC_LIBRARIES := android_support
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes) $(llvm_libc++_includes)
LOCAL_CFLAGS := $(llvm_libc++_cflags)
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
LOCAL_STATIC_LIBRARIES := android_support
include $(BUILD_SHARED_LIBRARY)

endif # LIBCXX_FORCE_REBUILD == true

$(call import-module, android/support)
