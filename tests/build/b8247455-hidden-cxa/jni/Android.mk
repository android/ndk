# This test is to demostrate the issue:
#
#  hidden symbol '__cxa_begin_cleanup' in ./obj/local/armeabi/libgnustl_static.a(eh_arm.o)
#    is referenced by DSO ./obj/local/armeabi/libidiv.so
#  hidden symbol '__cxa_type_match' in ./obj/local/armeabi/libgnustl_static.a(eh_arm.o)
#    is referenced by DSO ./obj/local/armeabi/libidiv.so
#
#  File idiv.cpp contains code potentially causes divide-by-zero exception. libidiv.so
#  is built with libgnustl_static.a which provides __cxa_begin_cleanup and
#  __cxa_type_match needed by unwinder in libgcc.a.  Unfortunately both are built
#  with hidden visibility, and causes warnings as the above when libidiv.so is used
#  to link other binaries.
#
# The fix is to unhide both __cxa_begin_cleanup and __cxa_type_match
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libidiv
LOCAL_SRC_FILES:= idiv.cpp
LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libthrow
LOCAL_SRC_FILES:= throw.cpp
LOCAL_CFLAGS := -Wall -Werror -frtti -fexceptions
LOCAL_SHARED_LIBRARIES = libidiv
include $(BUILD_SHARED_LIBRARY)