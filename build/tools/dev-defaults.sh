# Default values used by several dev-scripts.
#

# Current list of platform levels we support
#
# Note: levels 6 and 7 are omitted since they have the same native
# APIs as level 5. Same for levels 10, 11 and 12
#
API_LEVELS="3 4 5 8 9 14"

# Default ABIs for the target prebuilt binaries.
PREBUILT_ABIS="armeabi armeabi-v7a x86"

# Location of the STLport sources, relative to the NDK root directory
STLPORT_SUBDIR=sources/cxx-stl/stlport

# Location of the GAbi++ sources, relative to the NDK root directory
GABIXX_SUBDIR=sources/cxx-stl/gabi++

# Location of the GNU libstdc++ headers and libraries, relative to the NDK
# root directory.
GNUSTL_SUBDIR=sources/cxx-stl/gnu-libstdc++

# The date to use when downloading toolchain sources from AOSP servers
# Leave it empty for tip of tree.
TOOLCHAIN_GIT_DATE=2011-02-23

DEFAULT_GCC_VERSION=4.4.3
DEFAULT_BINUTILS_VERSION=2.20.1
DEFAULT_GDB_VERSION=6.6
DEFAULT_MPFR_VERSION=2.4.1
DEFAULT_GMP_VERSION=4.2.4

# Default platform to build target binaries against.
DEFAULT_PLATFORM=android-9

# The list of default CPU architectures we support
DEFAULT_ARCHS="arm x86"

# Default toolchain names and prefix
#
# This is used by get_default_toolchain_name_for_arch and get_default_toolchain_prefix_for_arch
# defined below
DEFAULT_ARCH_TOOLCHAIN_arm=arm-linux-androideabi-$DEFAULT_GCC_VERSION
DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm=arm-linux-androideabi

DEFAULT_ARCH_TOOLCHAIN_x86=x86-$DEFAULT_GCC_VERSION
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86=i686-android-linux

# The list of default host NDK systems we support
DEFAULT_SYSTEMS="linux-x86 windows darwin-x86"

# Return default NDK ABI for a given architecture name
# $1: Architecture name
# Out: ABI name
get_default_abi_for_arch ()
{
    local RET
    case $1 in
        arm)
            RET="armeabi"
            ;;
        x86)
            RET="x86"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86"
            exit 1
            ;;
    esac
    echo "$RET"
}


# Retrieve the list of default ABIs supported by a given architecture
# $1: Architecture name
# Out: space-separated list of ABI names
get_default_abis_for_arch ()
{
    local RET
    case $1 in
        arm)
            RET="armeabi armeabi-v7a"
            ;;
        x86)
            RET="x86"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86"
            exit 1
            ;;
    esac
    echo "$RET"
}


# Return the default name for a given architecture
# $1: Architecture name
# Out: default arch-specific toolchain name (e.g. arm-linux-androideabi-$GCC_VERSION)
# Return empty for unknown arch
get_default_toolchain_name_for_arch ()
{
    eval echo "\$DEFAULT_ARCH_TOOLCHAIN_$1"
}

# Return the default toolchain program prefix for a given architecture
# $1: Architecture name
# Out: default arch-specific toolchain prefix (e.g. arm-linux-androideabi)
# Return empty for unknown arch
get_default_toolchain_prefix_for_arch ()
{
    eval echo "\$DEFAULT_ARCH_TOOLCHAIN_PREFIX_$1"
}

