# Default values used by several dev-scripts.
#

# Current list of platform levels we support
#
# Note: levels 6 and 7 are omitted since they have the same native
# APIs as level 5. Same for levels 10, 11 and 12
#
API_LEVELS="3 4 5 8 9 12 13 14 15 16 17 18 19"

# Default ABIs for the target prebuilt binaries.
PREBUILT_ABIS="armeabi armeabi-v7a x86 mips armeabi-v7a-hard"

# Location of the STLport sources, relative to the NDK root directory
STLPORT_SUBDIR=sources/cxx-stl/stlport

# Location of the GAbi++ sources, relative to the NDK root directory
GABIXX_SUBDIR=sources/cxx-stl/gabi++

# Location of the GNU libstdc++ headers and libraries, relative to the NDK
# root directory.
GNUSTL_SUBDIR=sources/cxx-stl/gnu-libstdc++

# Location of the LLVM libc++ headers and libraries, relative to the NDK
# root directory.
LIBCXX_SUBDIR=sources/cxx-stl/llvm-libc++

# Location of the libportable sources, relative to the NDK root directory
LIBPORTABLE_SUBDIR=sources/android/libportable

# Location of the gccunwind sources, relative to the NDK root directory
GCCUNWIND_SUBDIR=sources/android/gccunwind

# Location of the compiler-rt sources, relative to the NDK root directory
COMPILER_RT_SUBDIR=sources/android/compiler-rt

# Location of the support sources for libc++, relative to the NDK root directory
SUPPORT_SUBDIR=sources/android/support

# The date to use when downloading toolchain sources from AOSP servers
# Leave it empty for tip of tree.
TOOLCHAIN_GIT_DATE=now

# The space-separated list of all GCC versions we support in this NDK
DEFAULT_GCC_VERSION_LIST="4.6 4.8"

# The default GCC version for this NDK, i.e. the first item in
# $DEFAULT_GCC_VERSION_LIST
#
DEFAULT_GCC_VERSION=$(echo "$DEFAULT_GCC_VERSION_LIST" | tr ' ' '\n' | head -n 1)
# The default GCC version for "clang -gcc-toolchain", the latest item in
# $DEFAULT_GCC_VERSION_LIST
#
DEFAULT_LLVM_GCC_VERSION=$(echo "$DEFAULT_GCC_VERSION_LIST" | tr ' ' '\n' | tail -n 1)

DEFAULT_BINUTILS_VERSION=2.21
DEFAULT_GDB_VERSION=7.3.x
DEFAULT_MPFR_VERSION=3.1.1
DEFAULT_GMP_VERSION=5.0.5
DEFAULT_MPC_VERSION=1.0.1
DEFAULT_CLOOG_VERSION=0.18.0
DEFAULT_ISL_VERSION=0.11.1
DEFAULT_PPL_VERSION=1.0
DEFAULT_PYTHON_VERSION=2.7.5
DEFAULT_PERL_VERSION=5.16.2

RECENT_BINUTILS_VERSION=2.23

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

DEFAULT_ARCH_TOOLCHAIN_NAME_arm64=aarch64-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_arm64=aarch64-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_x86=x86
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86=i686-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_x86_64=x86_64
DEFAULT_ARCH_TOOLCHAIN_PREFIX_x86_64=x86_64-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_mips=mipsel-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips=mipsel-linux-android

DEFAULT_ARCH_TOOLCHAIN_NAME_mips64=mips64el-linux-android
DEFAULT_ARCH_TOOLCHAIN_PREFIX_mips64=mips64el-linux-android

# The space-separated list of all LLVM versions we support in NDK
DEFAULT_LLVM_VERSION_LIST="3.4 3.3"

# The default LLVM version (first item in the list)
DEFAULT_LLVM_VERSION=$(echo "$DEFAULT_LLVM_VERSION_LIST" | tr ' ' '\n' | head -n 1)

# The default URL to download the LLVM tar archive
DEFAULT_LLVM_URL="http://llvm.org/releases"

# The list of default host NDK systems we support
DEFAULT_SYSTEMS="linux-x86 windows darwin-x86"

# The default issue tracker URL
DEFAULT_ISSUE_TRACKER_URL="http://source.android.com/source/report-bugs.html"

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
        x86|x86_64|mips)
            RET="$1"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86 x86_64 mips"
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
            RET="armeabi armeabi-v7a armeabi-v7a-hard"
            ;;
        x86|x86_64|mips)
            RET="$1"
            ;;
        *)
            2> echo "ERROR: Unsupported architecture name: $1, use one of: arm x86 x86_64 mips"
            exit 1
            ;;
    esac
    echo "$RET"
}

# Return toolchain name for given architecture and GCC version
# $1: Architecture name (e.g. 'arm')
# $2: optional, GCC version (e.g. '4.6')
# Out: default arch-specific toolchain name (e.g. 'arm-linux-androideabi-$GCC_VERSION')
# Return empty for unknown arch
get_toolchain_name_for_arch ()
{
    if [ ! -z "$2" ] ; then
        eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}-$2\"
    else
        eval echo \"\${DEFAULT_ARCH_TOOLCHAIN_NAME_$1}\"
    fi
}

# Return the default toolchain name for a given architecture
# $1: Architecture name (e.g. 'arm')
# Out: default arch-specific toolchain name (e.g. 'arm-linux-androideabi-$DEFAULT_GCC_VERSION')
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
# Out: list of toolchain names for this arch (e.g. arm-linux-androideabi-4.6 arm-linux-androideabi-4.8)
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

# Return the binutils version to be used by default when
# building a given version of GCC. This is needed to ensure
# we use binutils-2.19 when building gcc-4.4.3 for ARM and x86,
# and later binutils in other cases (mips, or gcc-4.6+).
#
# Note that technically, we could use latest binutils for all versions of
# GCC, however, in NDK r7, we did build GCC 4.4.3 with binutils-2.20.1
# and this resulted in weird C++ debugging bugs. For NDK r7b and higher,
# binutils was reverted to 2.19, to ensure at least
# feature/bug compatibility.
#
# $1: toolchain with version numer (e.g. 'arm-linux-androideabi-4.6')
#
get_default_binutils_version_for_gcc ()
{
    case $1 in
        mipsel-*-4.4.3|*-4.6) echo "$DEFAULT_BINUTILS_VERSION";;
        *-4.4.3) echo "2.19";;
        x86*-4.7) echo "2.23";;  # Use 2.23 to get x32 support in ld.gold
        *-4.7) echo "2.22";;
        *) echo "2.23";;
    esac
}

# Return the binutils version to be used by default when
# building a given version of llvm. For llvm-3.4 or later,
# we use binutils-2.23 to ensure the LLVMgold.so could be
# built properly. For llvm-3.3, we use binutils-2.21 as default.
#
# $1: toolchain with version numer (e.g. 'llvm-3.3')
#
get_default_binutils_version_for_llvm ()
{
    case $1 in
        *-3.3|*-3.2) echo "2.21";;
        *-3.4) echo "2.23";;
        *) echo "2.23";;
    esac
}

# Return the gdb version to be used by default when building a given
# version of GCC.
#
# $1: toolchain with version numer (e.g. 'arm-linux-androideabi-4.6')
#
get_default_gdb_version_for_gcc ()
{
    case $1 in
        x86*|aarch64-*) echo "7.6";;
        *) echo "$DEFAULT_GDB_VERSION";;
    esac
}
