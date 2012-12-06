LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := variadic73
LOCAL_SRC_FILES := variadic73.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := alias1
LOCAL_SRC_FILES := alias1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := cond1
LOCAL_SRC_FILES := cond1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := crossjump1
LOCAL_SRC_FILES := crossjump1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ctor1
LOCAL_SRC_FILES := ctor1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ctor2
LOCAL_SRC_FILES := ctor2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := delayslot1
LOCAL_SRC_FILES := delayslot1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := dtor1
LOCAL_SRC_FILES := dtor1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ehopt1
LOCAL_SRC_FILES := ehopt1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := elide1
LOCAL_SRC_FILES := elide1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := elide2
LOCAL_SRC_FILES := elide2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := filter1
LOCAL_SRC_FILES := filter1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := filter2
LOCAL_SRC_FILES := filter2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := fp-regs
LOCAL_SRC_FILES := fp-regs.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := gcsec1
LOCAL_SRC_FILES := gcsec1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ia64-2
LOCAL_SRC_FILES := ia64-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := loop1
LOCAL_SRC_FILES := loop1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := loop2
LOCAL_SRC_FILES := loop2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
#include $(CLEAR_VARS)
#LOCAL_MODULE := new1
#LOCAL_SRC_FILES := new1.cpp
#LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
#include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := omit-frame-pointer2
LOCAL_SRC_FILES := omit-frame-pointer2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr29166
LOCAL_SRC_FILES := pr29166.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := registers1
LOCAL_SRC_FILES := registers1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spbp
LOCAL_SRC_FILES := spbp.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec3
LOCAL_SRC_FILES := spec3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec5
LOCAL_SRC_FILES := spec5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
#include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec7
LOCAL_SRC_FILES := spec7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec9
LOCAL_SRC_FILES := spec9.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := synth2
LOCAL_SRC_FILES := synth2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := template1
LOCAL_SRC_FILES := template1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := unexpected1
LOCAL_SRC_FILES := unexpected1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := weak1
LOCAL_SRC_FILES := weak1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_assign
LOCAL_SRC_FILES := has_nothrow_assign.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_assign_odr
LOCAL_SRC_FILES := has_nothrow_assign_odr.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_constructor
LOCAL_SRC_FILES := has_nothrow_constructor.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_constructor_odr
LOCAL_SRC_FILES := has_nothrow_constructor_odr.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-1
LOCAL_SRC_FILES := has_nothrow_copy-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-2
LOCAL_SRC_FILES := has_nothrow_copy-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-3
LOCAL_SRC_FILES := has_nothrow_copy-3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-4
LOCAL_SRC_FILES := has_nothrow_copy-4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-5
LOCAL_SRC_FILES := has_nothrow_copy-5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-6
LOCAL_SRC_FILES := has_nothrow_copy-6.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy-7
LOCAL_SRC_FILES := has_nothrow_copy-7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++0x
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := has_nothrow_copy_odr
LOCAL_SRC_FILES := has_nothrow_copy_odr.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := label3
LOCAL_SRC_FILES := label3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pretty2
LOCAL_SRC_FILES := pretty2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := array5
LOCAL_SRC_FILES := array5.cpp
LOCAL_CFLAGS := -DPRINT
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ctor1_1
LOCAL_SRC_FILES := ctor1_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := delete2
LOCAL_SRC_FILES := delete2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new11
LOCAL_SRC_FILES := new11.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new16
LOCAL_SRC_FILES := new16.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new23
LOCAL_SRC_FILES := new23.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new5
LOCAL_SRC_FILES := new5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := placement2
LOCAL_SRC_FILES := placement2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ref9
LOCAL_SRC_FILES := ref9.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := alias2
LOCAL_SRC_FILES := alias2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh2
LOCAL_SRC_FILES := eh2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh3
LOCAL_SRC_FILES := eh3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr17697-1
LOCAL_SRC_FILES := pr17697-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr17697-2
LOCAL_SRC_FILES := pr17697-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr22167
LOCAL_SRC_FILES := pr22167.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr23299
LOCAL_SRC_FILES := pr23299.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pr36449
LOCAL_SRC_FILES := pr36449.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := reg-stack
LOCAL_SRC_FILES := reg-stack.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := unroll1
LOCAL_SRC_FILES := unroll1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-alloca-1
LOCAL_SRC_FILES := eh-alloca-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-global-1
LOCAL_SRC_FILES := eh-global-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-inline-1
LOCAL_SRC_FILES := eh-inline-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-inline-2
LOCAL_SRC_FILES := eh-inline-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-vararg-1
LOCAL_SRC_FILES := eh-vararg-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh-vararg-2
LOCAL_SRC_FILES := eh-vararg-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := throw-1
LOCAL_SRC_FILES := throw-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := throw-2
LOCAL_SRC_FILES := throw-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := throw-3
LOCAL_SRC_FILES := throw-3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := throw-4
LOCAL_SRC_FILES := throw-4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := arraynew
LOCAL_SRC_FILES := arraynew.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := vtable2
LOCAL_SRC_FILES := vtable2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := delete1
LOCAL_SRC_FILES := delete1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh1
LOCAL_SRC_FILES := eh1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := badalloc1
LOCAL_SRC_FILES := badalloc1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch11
LOCAL_SRC_FILES := catch11.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch12
LOCAL_SRC_FILES := catch12.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch3
LOCAL_SRC_FILES := catch3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch3p
LOCAL_SRC_FILES := catch3p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch4
LOCAL_SRC_FILES := catch4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch4p
LOCAL_SRC_FILES := catch4p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch5
LOCAL_SRC_FILES := catch5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch5p
LOCAL_SRC_FILES := catch5p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch6
LOCAL_SRC_FILES := catch6.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch6p
LOCAL_SRC_FILES := catch6p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch7
LOCAL_SRC_FILES := catch7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch7p
LOCAL_SRC_FILES := catch7p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch8
LOCAL_SRC_FILES := catch8.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch8p
LOCAL_SRC_FILES := catch8p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch9
LOCAL_SRC_FILES := catch9.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catch9p
LOCAL_SRC_FILES := catch9p.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := catchptr1
LOCAL_SRC_FILES := catchptr1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := cleanup1_1
LOCAL_SRC_FILES := cleanup1_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := cleanup2
LOCAL_SRC_FILES := cleanup2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := flow1
LOCAL_SRC_FILES := flow1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := fntry1
LOCAL_SRC_FILES := fntry1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := inline2
LOCAL_SRC_FILES := inline2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new1_2
LOCAL_SRC_FILES := new1_2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new2_1
LOCAL_SRC_FILES := new2_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pdel1
LOCAL_SRC_FILES := pdel1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := pdel2
LOCAL_SRC_FILES := pdel2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ptr1
LOCAL_SRC_FILES := ptr1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ptrmem1_1
LOCAL_SRC_FILES := ptrmem1_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow1
LOCAL_SRC_FILES := rethrow1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow2
LOCAL_SRC_FILES := rethrow2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow3
LOCAL_SRC_FILES := rethrow3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow4
LOCAL_SRC_FILES := rethrow4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow5
LOCAL_SRC_FILES := rethrow5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rethrow6
LOCAL_SRC_FILES := rethrow6.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec1
LOCAL_SRC_FILES := spec1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec2
LOCAL_SRC_FILES := spec2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec3_1
LOCAL_SRC_FILES := spec3_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec4
LOCAL_SRC_FILES := spec4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := spec7_1
LOCAL_SRC_FILES := spec7_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
#include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := terminate1
LOCAL_SRC_FILES := terminate1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := terminate2
LOCAL_SRC_FILES := terminate2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := tmpl1
LOCAL_SRC_FILES := tmpl1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := unwind1
LOCAL_SRC_FILES := unwind1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := vbase1_1
LOCAL_SRC_FILES := vbase1_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := vbase2_1
LOCAL_SRC_FILES := vbase2_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := vbase4_1
LOCAL_SRC_FILES := vbase4_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := const
LOCAL_SRC_FILES := const.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new3
LOCAL_SRC_FILES := new3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := rvalue1
LOCAL_SRC_FILES := rvalue1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := operators23
LOCAL_SRC_FILES := operators23.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := operators27
LOCAL_SRC_FILES := operators27.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new1_3
LOCAL_SRC_FILES := new1_3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := dyncast1
LOCAL_SRC_FILES := dyncast1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := dyncast2_1
LOCAL_SRC_FILES := dyncast2_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := dyncast7
LOCAL_SRC_FILES := dyncast7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh10
LOCAL_SRC_FILES := eh10.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh12
LOCAL_SRC_FILES := eh12.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh14
LOCAL_SRC_FILES := eh14.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh16
LOCAL_SRC_FILES := eh16.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh17
LOCAL_SRC_FILES := eh17.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh18
LOCAL_SRC_FILES := eh18.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh21
LOCAL_SRC_FILES := eh21.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh23
LOCAL_SRC_FILES := eh23.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh24
LOCAL_SRC_FILES := eh24.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh25
LOCAL_SRC_FILES := eh25.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh26
LOCAL_SRC_FILES := eh26.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh27
LOCAL_SRC_FILES := eh27.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh28
LOCAL_SRC_FILES := eh28.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh29
LOCAL_SRC_FILES := eh29.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh3_1
LOCAL_SRC_FILES := eh3_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh31
LOCAL_SRC_FILES := eh31.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh33
LOCAL_SRC_FILES := eh33.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh34
LOCAL_SRC_FILES := eh34.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh35
LOCAL_SRC_FILES := eh35.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh36
LOCAL_SRC_FILES := eh36.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh37
LOCAL_SRC_FILES := eh37.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh38
LOCAL_SRC_FILES := eh38.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh39
LOCAL_SRC_FILES := eh39.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh40
LOCAL_SRC_FILES := eh40.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh41
LOCAL_SRC_FILES := eh41.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh42
LOCAL_SRC_FILES := eh42.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh44
LOCAL_SRC_FILES := eh44.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh47
LOCAL_SRC_FILES := eh47.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh48
LOCAL_SRC_FILES := eh48.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh49
LOCAL_SRC_FILES := eh49.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
#include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh5
LOCAL_SRC_FILES := eh5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh50
LOCAL_SRC_FILES := eh50.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh51
LOCAL_SRC_FILES := eh51.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh53
LOCAL_SRC_FILES := eh53.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh55
LOCAL_SRC_FILES := eh55.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh6
LOCAL_SRC_FILES := eh6.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
#include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh7
LOCAL_SRC_FILES := eh7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh8
LOCAL_SRC_FILES := eh8.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh9
LOCAL_SRC_FILES := eh9.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p11667
LOCAL_SRC_FILES := p11667.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p755
LOCAL_SRC_FILES := p755.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p755a
LOCAL_SRC_FILES := p755a.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p7912
LOCAL_SRC_FILES := p7912.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p8155
LOCAL_SRC_FILES := p8155.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := p9706
LOCAL_SRC_FILES := p9706.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := delete3
LOCAL_SRC_FILES := delete3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new1_4
LOCAL_SRC_FILES := new1_4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := array1_2
LOCAL_SRC_FILES := array1_2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := delete3_1
LOCAL_SRC_FILES := delete3_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh3_2
LOCAL_SRC_FILES := eh3_2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := init7
LOCAL_SRC_FILES := init7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new6
LOCAL_SRC_FILES := new6.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := new7
LOCAL_SRC_FILES := new7.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := singleton
LOCAL_SRC_FILES := singleton.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := vbase2_2
LOCAL_SRC_FILES := vbase2_2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := explarg1
LOCAL_SRC_FILES := explarg1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := fntry1_1
LOCAL_SRC_FILES := fntry1_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := overload12_1
LOCAL_SRC_FILES := overload12_1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eb50
LOCAL_SRC_FILES := eb50.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eb88
LOCAL_SRC_FILES := eb88.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh990323-1
LOCAL_SRC_FILES := eh990323-1.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh990323-2
LOCAL_SRC_FILES := eh990323-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh990323-3
LOCAL_SRC_FILES := eh990323-3.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh990323-4
LOCAL_SRC_FILES := eh990323-4.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := eh990323-5
LOCAL_SRC_FILES := eh990323-5.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
 
include $(CLEAR_VARS)
LOCAL_MODULE := ice990323-2
LOCAL_SRC_FILES := ice990323-2.cpp
LOCAL_CPPFLAGS := -frtti -fexceptions -std=gnu++98
include $(BUILD_EXECUTABLE)
