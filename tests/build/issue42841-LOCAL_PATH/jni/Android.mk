# Note that the line LOCAL_PATH ends with trailing space and
# cause strange error message reads:
#
#  make: *** No rule to make target /issue42841-LOCAL_PATH.c', needed by
#     obj/local/armeabi/objs/issue42841-LOCAL_PATH/issue42841-LOCAL_PATH.o'.  Stop.
#
LOCAL_PATH := $(call my-dir) 

include $(CLEAR_VARS)
LOCAL_MODULE := issue42841-LOCAL_PATH
LOCAL_SRC_FILES := issue42841-LOCAL_PATH.c
include $(BUILD_EXECUTABLE)
