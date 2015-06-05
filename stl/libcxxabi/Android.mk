LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libunwind_ndk
LOCAL_CLANG := true
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include ndk/stl/libcxx/include ndk/android_support/include
LOCAL_CPPFLAGS := -fexceptions -D__STDC_FORMAT_MACROS
LOCAL_RTTI_FLAG := -frtti
LOCAL_SRC_FILES_arm := \
    src/Unwind/libunwind.cpp \
    src/Unwind/Unwind-EHABI.cpp \
    src/Unwind/Unwind-sjlj.c \
    src/Unwind/UnwindLevel1.c \
    src/Unwind/UnwindLevel1-gcc-ext.c \
    src/Unwind/UnwindRegistersRestore.S \
    src/Unwind/UnwindRegistersSave.S \

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libc++abi_ndk
LOCAL_CLANG := true
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include ndk/stl/libcxx/include ndk/android_support/include
LOCAL_CPPFLAGS := -fexceptions -D__STDC_FORMAT_MACROS
LOCAL_CPPFLAGS_arm := -DLIBCXXABI_USE_LLVM_UNWINDER=1
LOCAL_CPPFLAGS_arm64 := -DLIBCXXABI_USE_LLVM_UNWINDER=0
LOCAL_CPPFLAGS_mips := -DLIBCXXABI_USE_LLVM_UNWINDER=0
LOCAL_CPPFLAGS_mips64 := -DLIBCXXABI_USE_LLVM_UNWINDER=0
LOCAL_CPPFLAGS_x86 := -DLIBCXXABI_USE_LLVM_UNWINDER=0
LOCAL_CPPFLAGS_x86_64 := -DLIBCXXABI_USE_LLVM_UNWINDER=0
LOCAL_RTTI_FLAG := -frtti
LOCAL_SRC_FILES := \
    src/abort_message.cpp \
    src/cxa_aux_runtime.cpp \
    src/cxa_default_handlers.cpp \
    src/cxa_demangle.cpp \
    src/cxa_exception.cpp \
    src/cxa_exception_storage.cpp \
    src/cxa_guard.cpp \
    src/cxa_handlers.cpp \
    src/cxa_new_delete.cpp \
    src/cxa_personality.cpp \
    src/cxa_thread_atexit.cpp \
    src/cxa_unexpected.cpp \
    src/cxa_vector.cpp \
    src/cxa_virtual.cpp \
    src/exception.cpp \
    src/private_typeinfo.cpp \
    src/stdexcept.cpp \
    src/typeinfo.cpp \

# When src/cxa_exception.cpp is compiled with Clang assembler
# __cxa_end_cleanup_impl, although marked as used, was discarded
# since it is used only in embedded assembly code.
# This caused the following warning when linking libc++.so:
# libc++_static.a(cxa_exception.o)(.text.__cxa_end_cleanup+0x2):
# warning: relocation refers to discarded section
# See also http://llvm.org/bugs/show_bug.cgi?id=21292.
LOCAL_CLANG_CFLAGS_arm += -no-integrated-as

include $(BUILD_STATIC_LIBRARY)
