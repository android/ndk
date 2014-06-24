libcxxabi_path := $(call my-dir)

libcxxabi_src_files := \
        libcxxabi/src/abort_message.cpp \
        libcxxabi/src/cxa_aux_runtime.cpp \
        libcxxabi/src/cxa_default_handlers.cpp \
        libcxxabi/src/cxa_demangle.cpp \
        libcxxabi/src/cxa_exception.cpp \
        libcxxabi/src/cxa_exception_storage.cpp \
        libcxxabi/src/cxa_guard.cpp \
        libcxxabi/src/cxa_handlers.cpp \
        libcxxabi/src/cxa_new_delete.cpp \
        libcxxabi/src/cxa_personality.cpp \
        libcxxabi/src/cxa_unexpected.cpp \
        libcxxabi/src/cxa_vector.cpp \
        libcxxabi/src/cxa_virtual.cpp \
        libcxxabi/src/exception.cpp \
        libcxxabi/src/private_typeinfo.cpp \
        libcxxabi/src/stdexcept.cpp \
        libcxxabi/src/typeinfo.cpp \
        libcxxabi/src/Unwind/libunwind.cpp \
        libcxxabi/src/Unwind/Unwind-EHABI.cpp \
        libcxxabi/src/Unwind/UnwindLevel1.c \
        libcxxabi/src/Unwind/UnwindLevel1-gcc-ext.c \
        libcxxabi/src/Unwind/UnwindRegistersRestore.S \
        libcxxabi/src/Unwind/UnwindRegistersSave.S \
        libcxxabi/src/Unwind/Unwind-sjlj.c

libcxxabi_c_includes := $(libcxxabi_path)/libcxxabi/include

