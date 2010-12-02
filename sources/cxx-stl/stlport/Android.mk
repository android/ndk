LOCAL_PATH := $(call my-dir)

# Normally, we distribute the NDK with prebuilt binaries of STLport
# in $LOCAL_PATH/<abi>/. However,
#

STLPORT_FORCE_REBUILD := $(strip $(STLPORT_FORCE_REBUILD))
ifndef STLPORT_FORCE_REBUILD
  ifeq (,$(strip $(wildcard $(LOCAL_PATH)/libs/armeabi/libstlport_static.a)))
    $(call __ndk_info,WARNING: Rebuilding STLport libraries from sources!)
    $(call __ndk_info,You might want to use $$NDK/build/tools/build-stlport.sh)
    $(call __ndk_info,in order to build prebuilt versions to speed up your builds!)
    STLPORT_FORCE_REBUILD := true
  endif
endif

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

# Note: For now, this implementation depends on the system libstdc++
#       We may want to avoid that in the future, i.e. in order to
#       properly support exceptions and RTTI.
libstlport_static_libs := libstdc++

ifneq ($(STLPORT_FORCE_REBUILD),true)

$(call ndk_log,Using prebuilt STLport libraries)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_static
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).a
LOCAL_STATIC_LIBRARIES := $(libstlport_static_libs)
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_shared
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/lib$(LOCAL_MODULE).so
LOCAL_STATIC_LIBRARIES := $(libstlport_static_libs)
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
include $(PREBUILT_SHARED_LIBRARY)

else # STLPORT_FORCE_REBUILD == true

$(call ndk_log,Rebuilding STLport libraries from sources)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_static
LOCAL_SRC_FILES := $(libstlport_src_files)
LOCAL_CFLAGS := $(libstlport_cflags)
LOCAL_CPPFLAGS := $(libstlport_cppflags)
LOCAL_C_INCLUDES := $(libstlport_c_includes)
LOCAL_STATIC_LIBRARIES := $(libstlport_static_libs)
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := stlport_shared
LOCAL_SRC_FILES := $(libstlport_src_files)
LOCAL_CFLAGS := $(libstlport_cflags)
LOCAL_CPPFLAGS := $(libstlport_cppflags)
LOCAL_C_INCLUDES := $(libstlport_c_includes)
LOCAL_STATIC_LIBRARIES := $(libstlport_static_libs)
LOCAL_EXPORT_C_INCLUDES := $(libstlport_c_includes)
include $(BUILD_SHARED_LIBRARY)

endif # STLPORT_FORCE_REBUILD == true

# See above not above libstdc++ dependency.
$(call import-module,cxx-stl/system)
