ifeq (,$(filter %mips64,$(TARGET_ARCH_ABI)))

include $(NDK_ROOT)/sources/cxx-stl/gabi++/tests/Android.mk

else

# mips64 fails to compile.
# /s/mydroid/ndk/sources/cxx-stl/gabi++/tests/catch_const_pointer_nullptr.cpp: In function 'void test5()':
# /s/mydroid/ndk/sources/cxx-stl/gabi++/tests/catch_const_pointer_nullptr.cpp:81:6: error: dominator of 3 should be 6, not 4
#  void test5()
#       ^
# /s/mydroid/ndk/sources/cxx-stl/gabi++/tests/catch_const_pointer_nullptr.cpp:81:6: internal compiler error: in verify_dominators, at dominance.c:1047
# Please submit a full bug report,
# with preprocessed source if appropriate.
# See <http://source.android.com/source/report-bugs.html> for instructions.
# make: *** [obj/local/mips64/objs/test_gabixx_shared_catch_const_pointer_nullptr/catch_const_pointer_nullptr.o] Error 1

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := dummy
LOCAL_SRC_FILES := dummy.c
include $(BUILD_EXECUTABLE)

endif