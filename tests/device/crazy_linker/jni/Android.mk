# Define CRAZY_LINKER_SRC_DIR in your environment to redirect to a different
# version of the sources. This is useful if you have your own copy of the
# sources in a non-default location.
CRAZY_LINKER_SRC_DIR ?= $(NDK_ROOT)/sources/android/crazy_linker

include $(CRAZY_LINKER_SRC_DIR)/tests/Android.mk
