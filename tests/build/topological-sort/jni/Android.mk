# Used to check that ndk-build does a proper topological sort of
# module dependencies.
#
# Here's how this works:
#
#   1/ First, define a static library module named 'foo' with two source
#      files (foo.c and foo2.c), which implement two functions
#      (foo and foo2 respectively).
#
#   2/ Second, define another static library named 'bar' that depends on
#      module 'foo' but only uses the 'foo2' function from it.
#
#   3/ Third, define an executable that depends on both 'foo' and 'bar',
#      but only calls the 'foo' and 'bar' function (i.e. not foo2).
#      Order is important, i.e. it should have a line that says:
#
#       LOCAL_STATIC_LIBRARIES := foo bar
#
#      With NDK r8b and earlier, the final link command for the executable
#      will be like:
#
#        <linker> -o <executable> main.o libfoo.a libbar.a
#
#      Due to the way the linker works, this will fail. More specifically,
#      when trying to add bar.o to the final image, it will not be able to
#      find an object file that contains foo2(), because the search starts
#      _after_ libbar.a on the command-line.
#
#  With a NDK that implements correct topological dependency ordering,
#  the link line should be instead:
#
#        <linker> -o <executable> main.o libbar.a libfoo.a
#
#  Which will link, and work, correctly.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := foo
LOCAL_SRC_FILES := \
    foo.c \
    foo2.c
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bar
LOCAL_SRC_FILES := bar.c
LOCAL_STATIC_LIBRARIES := foo
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := test_topological_sort
LOCAL_SRC_FILES := main.c
# IMPORTANT: foo must appear before bar here.
LOCAL_STATIC_LIBRARIES := foo bar
include $(BUILD_EXECUTABLE)
