# We want to build support for all ARM ABIs
# This can be overriden by build/tools/rebuild-stlport.sh though
APP_ABI := armeabi armeabi-v7a
STLPORT_FORCE_REBUILD := true