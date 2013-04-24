APP_ABI := all

# This is a different with test-googletest-stlport:
# target API level 9 since it allows the use of clone() and regex()
# in the GoogleTest implementation.
#
APP_PLATFORM := android-9

APP_MODULES := \
    googletest_sample_1 \
    googletest_sample_2 \
    googletest_sample_3 \
    googletest_sample_4 \
    googletest_sample_5 \
    googletest_sample_6 \
    googletest_sample_7 \
    googletest_sample_8 \
    googletest_sample_9 \

APP_STL := gnustl_shared
