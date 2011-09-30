LOCAL_PATH:= $(call my-dir)

libgabi++_common_src_files := \
	src/array_type_info.cc \
	src/class_type_info.cc \
        src/delete.cc \
	src/dynamic_cast.cc \
	src/enum_type_info.cc \
	src/function_type_info.cc \
        src/new.cc \
	src/pbase_type_info.cc \
	src/pointer_type_info.cc \
	src/pointer_to_member_type_info.cc \
	src/si_class_type_info.cc \
	src/type_info.cc \
	src/vmi_class_type_info.cc

libgabi++_c_includes := $(LOCAL_PATH)/include

ifneq (,$(GABIXX_FORCE_REBUILD))

  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_shared
  LOCAL_SRC_FILES:= $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/libgabi++_shared
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_EXPORT_LDLIBS := -lstdc++
  LOCAL_CPP_FEATURES := rtti
  include $(PREBUILT_SHARED_LIBRARY)

  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_static
  LOCAL_SRC_FILES:= $(LOCAL_PATH)/libs/$(TARGET_ARCH_ABI)/libgabi++_static
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_EXPORT_LDLIBS := -lstdc++
  LOCAL_CPP_FEATURES := rtti
  include $(PREBUILT_STATIC_LIBRARY)

else # ! GABIXX_FORCE_REBUILD

  # Shared version of the library
  # Note that the module is named libgabi++_shared to avoid
  # any conflict with any potential system library named libgabi++
  #
  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_shared
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_SRC_FILES:= $(libgabi++_common_src_files)
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_EXPORT_LDLIBS := -lstdc++
  LOCAL_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti
  include $(BUILD_SHARED_LIBRARY)

  # And now the static version
  #
  include $(CLEAR_VARS)
  LOCAL_MODULE:= libgabi++_static
  LOCAL_SRC_FILES:= $(libgabi++_common_src_files)
  LOCAL_CPP_EXTENSION := .cc
  LOCAL_EXPORT_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_EXPORT_LDLIBS := -lstdc++
  LOCAL_C_INCLUDES := $(libgabi++_c_includes)
  LOCAL_CPP_FEATURES := rtti
  include $(BUILD_STATIC_LIBRARY)

endif # ! GABIXX_FORCE_REBUILD

