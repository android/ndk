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

libcxx_includes := $(LOCAL_PATH)/libcxx/include
libcxx_export_includes := $(libcxx_includes)
libcxx_sources := \
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

libcxx_sources := $(libcxx_sources:%=libcxx/src/%)

# For now, this library can only be used to build C++11 binaries.
libcxx_export_cxxflags := -std=c++11

ifeq (,$(filter clang%,$(NDK_TOOLCHAIN_VERSION)))
# Add -fno-strict-aliasing because __list_imp::_end_ breaks TBAA rules by declaring
# simply as __list_node_base then casted to __list_node derived from that.  See
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61571 for details
libcxx_export_cxxflags += -fno-strict-aliasing
endif

libcxx_cxxflags := $(libcxx_export_cxxflags)
libcxx_cflags := -D__STDC_FORMAT_MACROS

ifneq ($(LIBCXX_FORCE_REBUILD),true)

$(call ndk_log,Using prebuilt libc++ libraries)

android_support_c_includes := $(LOCAL_PATH)/../../android/support/include

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_LIB_EXTENSION)
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes) $(android_support_c_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE)$(TARGET_SONAME_EXTENSION)
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes) $(android_support_c_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
include $(PREBUILT_SHARED_LIBRARY)

else
# LIBCXX_FORCE_REBUILD == true

$(call ndk_log,Rebuilding libc++ libraries from sources)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := $(libcxx_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes) $(libcxx_includes)
LOCAL_CFLAGS := $(libcxx_cflags)
LOCAL_CPPFLAGS := $(libcxx_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_STATIC_LIBRARIES := libc++abi android_support
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_shared
LOCAL_WHOLE_STATIC_LIBRARIES := c++_static
LOCAL_EXPORT_C_INCLUDES := $(libcxx_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(libcxx_export_cxxflags)
LOCAL_STATIC_LIBRARIES := libc++abi android_support

# We use the LLVM unwinder for all the 32-bit ARM targets.
ifneq (,$(filter armeabi%,$(TARGET_ARCH_ABI)))
    LOCAL_STATIC_LIBRARIES += libunwind
endif

# But only need -latomic for armeabi.
ifeq ($(TARGET_ARCH_ABI),armeabi)
    LOCAL_LDLIBS := -latomic
endif
include $(BUILD_SHARED_LIBRARY)

endif # LIBCXX_FORCE_REBUILD == true

$(call import-module, android/support)
$(call import-module, cxx-stl/llvm-libc++abi)
