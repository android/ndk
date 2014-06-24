# This file is dual licensed under the MIT and the University of Illinois Open
# Source Licenses. See LICENSE.TXT for details.

LOCAL_PATH := $(call my-dir)

# Normally, we distribute the NDK with prebuilt binaries of libc++
# in $LOCAL_PATH/libs/<abi>/. However,
#

LIBCXX_FORCE_REBUILD := $(strip $(LIBCXX_FORCE_REBUILD))

__libcxx_force_rebuild := $(LIBCXX_FORCE_REBUILD)

ifndef LIBCXX_FORCE_REBUILD
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/libc++_static$(TARGET_LIB_EXTENSION))))
    $(call __ndk_info,WARNING: Rebuilding libc++ libraries from sources!)
    $(call __ndk_info,You might want to use $$NDK/build/tools/build-cxx-stl.sh --stl=libc++)
    $(call __ndk_info,in order to build prebuilt versions to speed up your builds!)
    __libcxx_force_rebuild := true
  endif
endif

# Use gabi++ for x86* and mips* until libc++/libc++abi is ready for them
ifneq (,$(filter x86% mips%,$(TARGET_ARCH_ABI)))
  __prebuilt_libcxx_compiled_with_gabixx := true
else
  __prebuilt_libcxx_compiled_with_gabixx := false
endif

__libcxx_use_gabixx := $(__prebuilt_libcxx_compiled_with_gabixx)

LIBCXX_USE_GABIXX := $(strip $(LIBCXX_USE_GABIXX))
ifeq ($(LIBCXX_USE_GABIXX),true)
  __libcxx_use_gabixx := true
endif

ifneq ($(__libcxx_use_gabixx),$(__prebuilt_libcxx_compiled_with_gabixx))
  ifneq ($(__libcxx_force_rebuild),true)
    ifeq ($(__prebuilt_libcxx_compiled_with_gabixx),true)
      $(call __ndk_info,WARNING: Rebuilding libc++ libraries from sources since libc++ prebuilt libraries for $(TARGET_ARCH_ABI))
      $(call __ndk_info,are compiled with gabi++ but LIBCXX_USE_GABIXX is not set to true)
    else
      $(call __ndk_info,WARNING: Rebuilding libc++ libraries from sources since libc++ prebuilt libraries for $(TARGET_ARCH_ABI))
      $(call __ndk_info,are not compiled with gabi++ and LIBCXX_USE_GABIXX is set to true)
    endif
    __libcxx_force_rebuild := true
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

ifeq ($(__libcxx_use_gabixx),true)

# Gabi++ emulates libcxxabi when building libcxx.
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

else
# libc++abi

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
llvm_libc++_sources += $(addprefix $(libcxxabi_sources_prefix:%/=%)/,$(libcxxabi_src_files))
llvm_libc++_includes += $(libcxxabi_c_includes)
llvm_libc++_export_includes += $(libcxxabi_c_includes)

endif

ifneq ($(__libcxx_force_rebuild),true)

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
# __libcxx_force_rebuild == true

$(call ndk_log,Rebuilding libc++ libraries from sources)

include $(CLEAR_VARS)
LOCAL_MODULE := c++_static
LOCAL_SRC_FILES := $(llvm_libc++_sources)
LOCAL_C_INCLUDES := $(android_support_c_includes) $(llvm_libc++_includes)
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
LOCAL_CPPFLAGS := $(llvm_libc++_cxxflags)
LOCAL_CPP_FEATURES := rtti exceptions
LOCAL_EXPORT_C_INCLUDES := $(llvm_libc++_export_includes)
LOCAL_EXPORT_CPPFLAGS := $(llvm_libc++_export_cxxflags)
LOCAL_STATIC_LIBRARIES := android_support
# For armeabi's shared version of libc++ compiled by clang, we need compiler-rt or libatomic
# for __atomic_fetch_add_4.  Note that "clang -gcc-toolchain" uses gcc4.8's as/ld/libs, including
# libatomic (which is not available in gcc4.6)
#
# On the other hand, all prebuilt libc++ libaries at sources/cxx-stl/llvm-libc++/libs are
# compiled with "clang -gcc-toolchain *4.8*" with -latomic, such that uses of prebuilt
# libc++_shared.so don't automatically requires -latomic or compiler-rt, unless code does
# "#include <atomic>" where  __atomic_is_lock_free is needed for armeabi and mips
#
ifeq ($(TARGET_ARCH_ABI),armeabi)
ifneq (,$(filter clang%,$(NDK_TOOLCHAIN_VERSION)))
LOCAL_SHARED_LIBRARIES := compiler_rt_shared
endif
endif

include $(BUILD_SHARED_LIBRARY)

endif # __libcxx_force_rebuild == true

$(call import-module, android/support)
$(call import-module, android/compiler-rt)
