LOCAL_PATH := $(call my-dir)

libstlport_path := $(call my-dir)

libstlport_src_files := \
        src/dll_main.cpp \
        src/fstream.cpp \
        src/strstream.cpp \
        src/sstream.cpp \
        src/ios.cpp \
        src/stdio_streambuf.cpp \
        src/istream.cpp \
        src/ostream.cpp \
        src/iostream.cpp \
        src/codecvt.cpp \
        src/collate.cpp \
        src/ctype.cpp \
        src/monetary.cpp \
        src/num_get.cpp \
        src/num_put.cpp \
        src/num_get_float.cpp \
        src/num_put_float.cpp \
        src/numpunct.cpp \
        src/time_facets.cpp \
        src/messages.cpp \
        src/locale.cpp \
        src/locale_impl.cpp \
        src/locale_catalog.cpp \
        src/facets_byname.cpp \
        src/complex.cpp \
        src/complex_io.cpp \
        src/complex_trig.cpp \
        src/string.cpp \
        src/bitset.cpp \
        src/allocators.cpp \
        src/c_locale.c \
        src/cxa.c \

libstlport_cflags := -D_GNU_SOURCE
libstlport_cppflags := -fuse-cxa-atexit
libstlport_c_includes := $(libstlport_path)/stlport

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_static
LOCAL_SRC_FILES := $(libstlport_src_files)
LOCAL_CFLAGS := $(libstlport_cflags)
LOCAL_CPPFLAGS := $(libstlport_cppflags)
LOCAL_C_INCLUDES := $(libstlport_c_includes)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_shared
LOCAL_SRC_FILES := $(libstlport_src_files)
LOCAL_CFLAGS := $(libstlport_cflags)
LOCAL_CPPFLAGS := $(libstlport_cppflags)
LOCAL_C_INCLUDES := $(libstlport_c_includes)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
include $(BUILD_SHARED_LIBRARY)
