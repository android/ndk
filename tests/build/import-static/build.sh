cd `dirname $0`
PWD=$(pwd)

# Update NDK_MODULE_PATH so we can find our imported modules
export NDK_MODULE_PATH="$PWD"

# Build everything
$NDK/ndk-build "$@"
