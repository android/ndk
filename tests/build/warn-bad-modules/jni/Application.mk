APP_ABI := all

# Android.mk only defines the 'foo' module.
# Itentionally ask for the non-existing 'bar' module to be built.
APP_MODULES := bar
