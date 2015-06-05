LOCAL_PATH := $(call my-dir)

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
libstlport_c_includes := $(LOCAL_PATH)/stlport

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_static
LOCAL_SRC_FILES := $(libstlport_src_files)
LOCAL_CFLAGS := $(libstlport_cflags)
LOCAL_CPPFLAGS := $(libstlport_cppflags) -fexceptions
LOCAL_C_INCLUDES := $(libstlport_c_includes)
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
LOCAL_WHOLE_STATIC_LIBRARIES := libgabi++_static
LOCAL_RTTI_FLAG := -frtti
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_shared
LOCAL_WHOLE_STATIC_LIBRARIES := stlport_static
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_LDLIBS := -ldl
include $(BUILD_SHARED_LIBRARY)
