# This is used to check that the internal unit tests of ndk-build
# work properly. Note that these only check internal Make functions
# within the build system, not anything that tries to build something.
cd $(dirname "$0")
$NDK/ndk-build NDK_UNIT_TESTS=1 clean
