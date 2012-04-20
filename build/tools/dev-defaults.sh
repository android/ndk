# Default values used by several dev-scripts.
#

# Current list of platform levels we support
#
# Note: levels 6 and 7 are omitted since they have the same native
# APIs as level 5. Same for levels 10, 11 and 12
#
API_LEVELS="3 4 5 8 9 14"

# Default ABIs for the target prebuilt binaries.
PREBUILT_ABIS="armeabi armeabi-v7a x86 mips"

# Location of the STLport sources, relative to the NDK root directory
STLPORT_SUBDIR=sources/cxx-stl/stlport

# Location of the GAbi++ sources, relative to the NDK root directory
GABIXX_SUBDIR=sources/cxx-stl/gabi++

# Location of the GNU libstdc++ headers and libraries, relative to the NDK
# root directory.
GNUSTL_SUBDIR=sources/cxx-stl/gnu-libstdc++

# The date to use when downloading toolchain sources from AOSP servers
# Leave it empty for tip of tree.
TOOLCHAIN_GIT_DATE=2012-04-19

# The space-separated list of all GCC versions we support in this NDK
DEFAULT_GCC_VERSION_LIST="4.4.3"

# The default GCC version for this NDK, i.e. the first item in
# $DEFAULT_GCC_VERSION_LIST
#
DEFAULT_GCC_VERSION=$(echo "$DEFAULT_GCC_VERSION_LIST" | tr ' ' '\n' | head -n 1)

DEFAULT_BINUTILS_VERSION=2.19
DEFAULT_GDB_VERSION=6.6
DEFAULT_MPFR_VERSION=2.4.1
DEFAULT_GMP_VERSION=4.2.4
DEFAULT_MPC_VERSION=0.8.1

# Default platform to build target binaries against.
DEFAULT_PLATFORM=android-9

# The list of default CPU architectures we support
DEFAULT_ARCHS="arm x86 mips"

# Default toolchain names and prefix
#
# This is used by get_default_toolchain_name_for_arch and get_default_toolchain_prefix_for_arch
# defined below
DEFAULT_ARCH_TOOLCHAIN_NAME_arm=arm-linux-androideabi
DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm=arm-linux-androideabi

DEFAULT_ARCH_TOOLCHAIN_NAME_x86=x86
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86=i686-android-linux

DEFAULT_ARCH_TOOLCHAIN_NAME_mips=mipsel-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips=mipsel-linux-android

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
        mips)
            RET="mips"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86 mips"
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
        mips)
            RET="mips"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86 mips"
            exit 1
            ;;
    esac
    echo "$RET"
}


# Return the default name for a given architecture
# $1: Architecture name (e.g. 'arm')
# Out: default arch-specific toolchain name (e.g. 'arm-linux-androideabi-$GCC_VERSION')
# Return empty for unknown arch
get_default_toolchain_name_for_arch ()
{
    eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}-$DEFAULT_GCC_VERSION\"
}

# Return the default toolchain program prefix for a given architecture
# $1: Architecture name
# Out: default arch-specific toolchain prefix (e.g. arm-linux-androideabi)
# Return empty for unknown arch
get_default_toolchain_prefix_for_arch ()
{
    eval echo "\$DEFAULT_ARCH_TOOLCHAIN_PREFIX_$1"
}

# Get the list of all toolchain names for a given architecture
# $1: architecture (e.g. 'arm')
# Out: list of toolchain names for this arch (e.g. arm-linux-androideabi-4.6 arm-linux-androideabi-4.4.3)
# Return empty for unknown arch
get_toolchain_name_list_for_arch ()
{
    local PREFIX VERSION RET
    PREFIX=$(eval echo \"\$DEFAULT_ARCH_TOOLCHAIN_NAME_$1\")
    if [ -z "$PREFIX" ]; then
        return 0
    fi
    RET=""
    for VERSION in $DEFAULT_GCC_VERSION_LIST; do
        RET=$RET" $PREFIX-$VERSION"
    done
    RET=${RET## }
    echo "$RET"
}
