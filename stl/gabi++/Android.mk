LOCAL_PATH:= $(call my-dir)

libgabi++_src_files := \
    src/array_type_info.cc \
    src/auxilary.cc \
    src/class_type_info.cc \
    src/cxxabi.cc \
    src/cxxabi_vec.cc \
    src/delete.cc \
    src/dwarf_helper.cc \
    src/dynamic_cast.cc \
    src/enum_type_info.cc \
    src/exception.cc \
    src/fatal_error.cc \
    src/function_type_info.cc \
    src/fundamental_type_info.cc \
    src/helper_func_internal.cc \
    src/new.cc \
    src/one_time_construction.cc \
    src/pbase_type_info.cc \
    src/personality.cc \
    src/pointer_type_info.cc \
    src/pointer_to_member_type_info.cc \
    src/call_unexpected.cc \
    src/si_class_type_info.cc \
    src/stdexcept.cc \
    src/terminate.cc \
    src/type_info.cc \
    src/vmi_class_type_info.cc \

libgabi++_c_includes := $(LOCAL_PATH)/include

include $(CLEAR_VARS)
LOCAL_MODULE:= libgabi++_static
LOCAL_CPP_EXTENSION := .cc
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_SRC_FILES:= $(libgabi++_src_files)
LOCAL_CPP_EXTENSION := .cc
LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
LOCAL_C_INCLUDES := $(libgabi++_c_includes)
LOCAL_CPPFLAGS := -fexceptions
LOCAL_RTTI_FLAG := -frtti
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libgabi++_shared
LOCAL_CPP_EXTENSION := .cc
LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := none
LOCAL_WHOLE_STATIC_LIBRARIES := libgabi++_static
LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
LOCAL_LDLIBS := -ldl
include $(BUILD_SHARED_LIBRARY)
