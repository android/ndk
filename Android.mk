# Please try to keep this file empty by only including what is necessary to
# build the Android platform. This is not the normal usage of the NDK.

ifneq ($(TARGET_SIMULATOR),true)
include ndk/sources/android/cpufeatures/Android.mk
endif
