libgabi++_path := $(call my-dir)

libgabi++_src_files := \
        src/array_type_info.cc \
        src/class_type_info.cc \
        src/delete.cc \
        src/dynamic_cast.cc \
        src/enum_type_info.cc \
        src/function_type_info.cc \
        src/new.cc \
        src/one_time_construction.cc \
        src/pbase_type_info.cc \
        src/pointer_type_info.cc \
        src/pointer_to_member_type_info.cc \
        src/si_class_type_info.cc \
        src/type_info.cc \
        src/vmi_class_type_info.cc

libgabi++_c_includes := $(libgabi++_path)/include
