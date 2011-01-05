# Common functions for all prebuilt-related scripts
# This is included/sourced by other scripts
#

. `dirname $0`/../core/ndk-common.sh

#====================================================
#
#  UTILITY FUNCTIONS
#
#====================================================

# Return the maximum length of a series of strings
#
# Usage:  len=`max_length <string1> <string2> ...`
#
max_length ()
{
    echo "$@" | tr ' ' '\n' | awk 'BEGIN {max=0} {len=length($1); if (len > max) max=len} END {print max}'
}

# Translate dashes to underscores
# Usage:  str=`dashes_to_underscores <values>`
dashes_to_underscores ()
{
    echo $@ | tr '-' '_'
}

# Translate underscores to dashes
# Usage: str=`underscores_to_dashes <values>`
underscores_to_dashes ()
{
    echo $@ | tr '_' '-'
}

#====================================================
#
#  OPTION PROCESSING
#
#====================================================

# We recognize the following option formats:
#
#  -f
#  --flag
#
#  -s<value>
#  --setting=<value>
#

# NOTE: We translate '-' into '_' when storing the options in global variables
#

OPTIONS=""
OPTION_FLAGS=""
OPTION_SETTINGS=""

# Set a given option attribute
# $1: option name
# $2: option attribute
# $3: attribute value
#
option_set_attr ()
{
    eval OPTIONS_$1_$2=\"$3\"
}

# Get a given option attribute
# $1: option name
# $2: option attribute
#
option_get_attr ()
{
    echo `var_value OPTIONS_$1_$2`
}

# Register a new option
# $1: option
# $2: small abstract for the option
# $3: optional. default value
#
register_option_internal ()
{
    optlabel=
    optname=
    optvalue=
    opttype=
    while [ -n "1" ] ; do
        # Check for something like --setting=<value>
        echo "$1" | grep -q -E -e '^--[^=]+=<.+>$'
        if [ $? = 0 ] ; then
            optlabel=`expr -- "$1" : '\(--[^=]*\)=.*'`
            optvalue=`expr -- "$1" : '--[^=]*=\(<.*>\)'`
            opttype="long_setting"
            break
        fi

        # Check for something like --flag
        echo "$1" | grep -q -E -e '^--[^=]+$'
        if [ $? = 0 ] ; then
            optlabel="$1"
            opttype="long_flag"
            break
        fi

        # Check for something like -f<value>
        echo "$1" | grep -q -E -e '^-[A-Za-z0-9]<.+>$'
        if [ $? = 0 ] ; then
            optlabel=`expr -- "$1" : '\(-.\).*'`
            optvalue=`expr -- "$1" : '-.\(<.+>\)'`
            opttype="short_setting"
            break
        fi

        # Check for something like -f
        echo "$1" | grep -q -E -e '^-.$'
        if [ $? = 0 ] ; then
            optlabel="$1"
            opttype="short_flag"
            break
        fi

        echo "ERROR: Invalid option format: $1"
        echo "       Check register_option call"
        exit 1
    done

    log "new option: type='$opttype' name='$optlabel' value='$optvalue'"

    optname=`dashes_to_underscores $optlabel`
    OPTIONS="$OPTIONS $optname"
    OPTIONS_TEXT="$OPTIONS_TEXT $1"
    option_set_attr $optname label "$optlabel"
    option_set_attr $optname otype "$opttype"
    option_set_attr $optname value "$optvalue"
    option_set_attr $optname text "$1"
    option_set_attr $optname abstract "$2"
    option_set_attr $optname default "$3"
}

# Register a new option with a function callback.
#
# $1: option
# $2: name of function that will be called when the option is parsed
# $3: small abstract for the option
# $4: optional. default value
#
register_option ()
{
    local optname optvalue opttype optlabel
    register_option_internal "$1" "$3" "$4"
    option_set_attr $optname funcname "$2"
}

# Register a new option with a variable store
#
# $1: option
# $2: name of variable that will be set by this option
# $3: small abstract for the option
#
# NOTE: The current value of $2 is used as the default
#
register_var_option ()
{
    local optname optvalue opttype optlabel
    register_option_internal "$1" "$3" "`var_value $2`"
    option_set_attr $optname varname "$2"
}


MINGW=no
do_mingw_option () { MINGW=yes; }

register_mingw_option ()
{
    if [ "$HOST_OS" = "linux" ] ; then
        register_option "--mingw" do_mingw_option "Generate windows binaries on Linux."
    fi
}

# Print the help, including a list of registered options for this program
# Note: Assumes PROGRAM_PARAMETERS and PROGRAM_DESCRIPTION exist and
#       correspond to the parameters list and the program description
#
print_help ()
{
    local opt text abstract default

    echo "Usage: $PROGNAME [options] $PROGRAM_PARAMETERS"
    echo ""
    if [ -n "$PROGRAM_DESCRIPTION" ] ; then
        echo "$PROGRAM_DESCRIPTION"
        echo ""
    fi
    echo "Valid options (defaults are in brackets):"
    echo ""

    maxw=`max_length "$OPTIONS_TEXT"`
    AWK_SCRIPT=`echo "{ printf \"%-${maxw}s\", \\$1 }"`
    for opt in $OPTIONS; do
        text=`option_get_attr $opt text | awk "$AWK_SCRIPT"`
        abstract=`option_get_attr $opt abstract`
        default=`option_get_attr $opt default`
        if [ -n "$default" ] ; then
            echo "  $text     $abstract [$default]"
        else
            echo "  $text     $abstract"
        fi
    done
    echo ""
}

option_panic_no_args ()
{
    echo "ERROR: Option '$1' does not take arguments. See --help for usage."
    exit 1
}

option_panic_missing_arg ()
{
    echo "ERROR: Option '$1' requires an argument. See --help for usage."
    exit 1
}

extract_parameters ()
{
    local opt optname otype value name fin funcname
    PARAMETERS=""
    while [ -n "$1" ] ; do
        # If the parameter does not begin with a dash
        # it is not an option.
        param=`expr -- "$1" : '^\([^\-].*\)$'`
        if [ -n "$param" ] ; then
            if [ -z "$PARAMETERS" ] ; then
                PARAMETERS="$1"
            else
                PARAMETERS="$PARAMETERS $1"
            fi
            shift
            continue
        fi

        while [ -n "1" ] ; do
            # Try to match a long setting, i.e. --option=value
            opt=`expr -- "$1" : '^\(--[^=]*\)=.*$'`
            if [ -n "$opt" ] ; then
                otype="long_setting"
                value=`expr -- "$1" : '^--[^=]*=\(.*\)$'`
                break
            fi

            # Try to match a long flag, i.e. --option
            opt=`expr -- "$1" : '^\(--.*\)$'`
            if [ -n "$opt" ] ; then
                otype="long_flag"
                value="yes"
                break
            fi

            # Try to match a short setting, i.e. -o<value>
            opt=`expr -- "$1" : '^\(-[A-Za-z0-9]\)..*$'`
            if [ -n "$opt" ] ; then
                otype="short_setting"
                value=`expr -- "$1" : '^-.\(.*\)$'`
                break
            fi

            # Try to match a short flag, i.e. -o
            opt=`expr -- "$1" : '^\(-.\)$'`
            if [ -n "$opt" ] ; then
                otype="short_flag"
                value="yes"
                break
            fi

            echo "ERROR: Unknown option '$1'. Use --help for list of valid values."
            exit 1
        done

        #echo "Found opt='$opt' otype='$otype' value='$value'"

        name=`dashes_to_underscores $opt`
        found=0
        for xopt in $OPTIONS; do
            if [ "$name" != "$xopt" ] ; then
                continue
            fi
            # Check that the type is correct here
            #
            # This also allows us to handle -o <value> as -o<value>
            #
            xotype=`option_get_attr $name otype`
            if [ "$otype" != "$xotype" ] ; then
                case "$xotype" in
                "short_flag")
                    option_panic_no_args $opt
                    ;;
                "short_setting")
                    if [ -z "$2" ] ; then
                        option_panic_missing_arg $opt
                    fi
                    value="$2"
                    shift
                    ;;
                "long_flag")
                    option_panic_no_args $opt
                    ;;
                "long_setting")
                    option_panic_missing_arg $opt
                    ;;
                esac
            fi
            found=1
            break
            break
        done
        if [ "$found" = "0" ] ; then
            echo "ERROR: Unknown option '$opt'. See --help for usage."
            exit 1
        fi
        # Set variable or launch option-specific function.
        varname=`option_get_attr $name varname`
        if [ -n "$varname" ] ; then
            eval ${varname}=\"$value\"
        else
            eval `option_get_attr $name funcname` \"$value\"
        fi
        shift
    done
}

do_option_help ()
{
    print_help
    exit 0
}

VERBOSE=no
VERBOSE2=no
do_option_verbose ()
{
    if [ $VERBOSE = "yes" ] ; then
        VERBOSE2=yes
    else
        VERBOSE=yes
    fi
}

register_option "--help"          do_option_help     "Print this help."
register_option "--verbose"       do_option_verbose  "Enable verbose mode."

#====================================================
#
#  TOOLCHAIN AND ABI PROCESSING
#
#====================================================

# Determine optional variable value
# $1: final variable name
# $2: option variable name
# $3: small description for the option
fix_option ()
{
    if [ -n "$2" ] ; then
        eval $1="$2"
        log "Using specific $3: $2"
    else
        log "Using default $3: `var_value $1`"
    fi
}


# If SYSROOT is empty, check that $1/$2 contains a sysroot
# and set the variable to it.
#
# $1: sysroot path
# $2: platform/arch suffix
check_sysroot ()
{
    if [ -z "$SYSROOT" ] ; then
        log "Probing directory for sysroot: $1/$2"
        if [ -d $1/$2 ] ; then
            SYSROOT=$1/$2
        fi
    fi
}

# Determine sysroot
# $1: Option value (or empty)
#
fix_sysroot ()
{
    if [ -n "$1" ] ; then
        eval SYSROOT="$1"
        log "Using specified sysroot: $1"
    else
        SYSROOT_SUFFIX=$PLATFORM/arch-$ARCH
        SYSROOT=
        check_sysroot $NDK_DIR/platforms $SYSROOT_SUFFIX
        check_sysroot $ANDROID_NDK_ROOT/platforms $SYSROOT_SUFFIX
        check_sysroot `dirname $ANDROID_NDK_ROOT`/development/ndk/platforms $SYSROOT_SUFFIX

        if [ -z "$SYSROOT" ] ; then
            echo "ERROR: Could not find NDK sysroot path for $SYSROOT_SUFFIX."
            echo "       Use --sysroot=<path> to specify one."
            exit 1
        fi
    fi

    if [ ! -f $SYSROOT/usr/include/stdlib.h ] ; then
        echo "ERROR: Invalid sysroot path: $SYSROOT"
        echo "       Use --sysroot=<path> to indicate a valid one."
        exit 1
    fi
}

# Use the check for the availability of a compatibility SDK in Darwin
# this can be used to generate binaries compatible with either Tiger or
# Leopard.
#
# $1: SDK root path
# $2: MacOS X minimum version (e.g. 10.4)
check_darwin_sdk ()
{
    if [ -d "$1" ] ; then
        HOST_CFLAGS="-isysroot $1 -mmacosx-version-min=$2 -DMAXOSX_DEPLOYEMENT_TARGET=$2"
        HOST_LDFLAGS="-Wl,-syslibroot,$sdk -mmacosx-version-min=$2"
        return 0  # success
    fi
    return 1
}


prepare_host_flags ()
{
    # detect build tag
    case $HOST_TAG in
        linux-x86)
            ABI_CONFIGURE_BUILD=i386-linux-gnu
            ;;
        linux-x86_64)
            ABI_CONFIGURE_BUILD=x86_64-linux-gnu
            ;;
        darwin-x86)
            ABI_CONFIGURE_BUILD=i686-apple-darwin
            ;;
        darwin-x86_64)
            ABI_CONFIGURE_BUILD=x86_64-apple-darwin
            ;;
        windows)
            ABI_CONFIGURE_BUILD=i686-pc-cygwin
            ;;
        *)
            echo "ERROR: Unsupported HOST_TAG: $HOST_TAG"
            echo "Please update 'prepare_host_flags' in build/tools/prebuilt-common.sh"
            ;;
    esac

    # By default, assume host == build
    ABI_CONFIGURE_HOST="$ABI_CONFIGURE_BUILD"

    # On Linux, detect our legacy-compatible toolchain when in the Android
    # source tree, and use it to force the generation of glibc-2.7 compatible
    # binaries.
    #
    # We only do this if the CC variable is not defined to a given value
    # and the --mingw option is not used.
    #
    if [ "$HOST_OS" = "linux" -a -z "$CC" -a "$MINGW" != "yes" ]; then
        LEGACY_TOOLCHAIN_DIR="$ANDROID_NDK_ROOT/../prebuilt/linux-x86/toolchain/i686-linux-glibc2.7-4.4.3"
        if [ -d "$LEGACY_TOOLCHAIN_DIR" ] ; then
            dump "Forcing generation of Linux binaries with legacy toolchain"
            CC="$LEGACY_TOOLCHAIN_DIR/bin/i686-linux-gcc"
            CXX="$LEGACY_TOOLCHAIN_DIR/bin/i686-linux-g++"
        fi
    fi

    # Force generation of 32-bit binaries on 64-bit systems
    CC=${CC:-gcc}
    CXX=${CXX:-g++}
    case $HOST_TAG in
        darwin-*)
            # Try to build with Tiger SDK if available
            if check_darwin_sdk /Developer/SDKs/MacOSX10.4.sdku 10.4; then
                log "Generating Tiger-compatible binaries!"
            # Otherwise with Leopard SDK
            elif check_darwin_sdk /Developer/SDKs/MacOSX10.5.sdk 10.5; then
                log "Generating Leopard-compatible binaries!"
            else
                local version=`sw_vers -productVersion`
                log "Generating $version-compatible binaries!"
            fi
            ;;
    esac

    # Force generation of 32-bit binaries on 64-bit systems.
    # We used to test the value of $HOST_TAG for *-x86_64, but this is
    # not sufficient on certain systems.
    #
    # For example, Snow Leopard can be booted with a 32-bit kernel, running
    # a 64-bit userland, with a compiler that generates 64-bit binaries by
    # default *even* though "gcc -v" will report --target=i686-apple-darwin10!
    #
    # So know, simply probe for the size of void* by performing a small runtime
    # compilation test.
    #
    cat > $TMPC <<EOF
    /* this test should fail if the compiler generates 64-bit machine code */
    int test_array[1-2*(sizeof(void*) != 4)];
EOF
    echo -n "Checking whether the compiler generates 32-bit binaries..."
    log $CC $HOST_CFLAGS -c -o $TMPO $TMPC
    $CC $HOST_CFLAGS -c -o $TMPO $TMPC >$TMPL 2>&1
    if [ $? != 0 ] ; then
        echo "no"
        # NOTE: We need to modify the definitions of CC and CXX directly
        #        here. Just changing the value of CFLAGS / HOST_CFLAGS
        #        will not work well with the GCC toolchain scripts.
        CC="$CC -m32"
        CXX="$CXX -m32"
    else
        echo "yes"
    fi

    # For now, we only support building 32-bit binaries anyway
    force_32bit_binaries  # to modify HOST_TAG and others
    HOST_GMP_ABI="32"

    # Now handle the --mingw flag
    if [ "$MINGW" = "yes" ] ; then
        case $HOST_TAG in
            linux-*)
                ;;
            *)
                echo "ERROR: Can only enable mingw on Linux platforms !"
                exit 1
                ;;
        esac
        ABI_CONFIGURE_HOST=i586-mingw32msvc
        HOST_OS=windows
        HOST_TAG=windows
        HOST_EXE=.exe
        # It turns out that we need to undefine this to be able to
        # perform a canadian-cross build with mingw. Otherwise, the
        # GMP configure scripts will not be called with the right options
        HOST_GMP_ABI=
    fi
}

parse_toolchain_name ()
{
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name!"
        exit 1
    fi

    ABI_CFLAGS_FOR_TARGET=
    ABI_CXXFLAGS_FOR_TARGET=

    # Determine ABI based on toolchain name
    #
    case "$TOOLCHAIN" in
    arm-eabi-*)
        ARCH="arm"
        ABI_CONFIGURE_TARGET="arm-eabi"
        ;;
    arm-linux-androideabi-*)
        ARCH="arm"
        ABI_CONFIGURE_TARGET="arm-linux-androideabi"
        ABI_CONFIGURE_EXTRA_FLAGS="--with-gmp-version=4.2.4 --with-mpfr-version=2.4.1
--with-arch=armv5te"
        # Disable ARM Gold linker for now, it doesn't build on Windows, it
        # crashes with SIGBUS on Darwin, and produces weird executables on
        # linux that strip complains about... Sigh.
        #ABI_CONFIGURE_EXTRA_FLAGS="$ABI_CONFIGURE_EXTRA_FLAGS --enable-gold=both/gold"

        # Enable C++ exceptions, RTTI and GNU libstdc++ at the same time
        # You can't really build these separately at the moment.
        ABI_CFLAGS_FOR_TARGET="-fexceptions"
        ABI_CXXFLAGS_FOR_TARGET="-frtti"
        ABI_CONFIGURE_EXTRA_FLAGS="$ABI_CONFIGURE_EXTRA_FLAGS --enable-libstdc__-v3"
        # Stick to 6.6 for now. 7.1.x doesn't seem to work right now.
        #GDB_VERSION=7.1.x
        ;;
    x86-*)
        ARCH="x86"
        ABI_INSTALL_NAME="x86"
        ABI_CONFIGURE_TARGET="i686-android-linux-gnu"
        PLATFORM=android-5
        ;;
    * )
        echo "Invalid toolchain specified. Expected (arm-eabi-*|x86-*)"
        echo ""
        print_help
        exit 1
        ;;
    esac

    log "Targetting CPU: $ARCH"

    GCC_VERSION=`expr -- "$TOOLCHAIN" : '.*-\([0-9\.]*\)'`
    log "Using GCC version: $GCC_VERSION"

    # Determine --host value when building gdbserver
    case "$TOOLCHAIN" in
    arm-*)
        GDBSERVER_HOST=arm-eabi-linux
        GDBSERVER_CFLAGS="-fno-short-enums"
        ;;
    x86-*)
        GDBSERVER_HOST=i686-android-linux-gnu
        GDBSERVER_CFLAGS=
        ;;
    esac

}

# Return the host/build specific path for prebuilt toolchain binaries
# relative to $1.
#
# $1: target root NDK directory
#
get_toolchain_install ()
{
    echo "$1/toolchains/$TOOLCHAIN/prebuilt/$HOST_TAG"
}


# Set the toolchain target NDK location.
# this sets TOOLCHAIN_PATH and TOOLCHAIN_PREFIX
# $1: target NDK path
set_toolchain_ndk ()
{
    TOOLCHAIN_PATH=`get_toolchain_install $1`
    log "Using toolchain path: $TOOLCHAIN_PATH"

    TOOLCHAIN_PREFIX=$TOOLCHAIN_PATH/bin/$ABI_CONFIGURE_TARGET
    log "Using toolchain prefix: $TOOLCHAIN_PREFIX"
}

# Check that a toolchain is properly installed at a target NDK location
#
# $1: target root NDK directory
#
check_toolchain_install ()
{
    TOOLCHAIN_PATH=`get_toolchain_install $1`
    if [ ! -d "$TOOLCHAIN_PATH" ] ; then
        echo "ERROR: Toolchain '$TOOLCHAIN' not installed in '$NDK_DIR'!"
        echo "       Ensure that the toolchain has been installed there before."
        exit 1
    fi

    set_toolchain_ndk $1
}


random_temp_directory ()
{
    mkdir -p /tmp/ndk-toolchain
    mktemp -d /tmp/ndk-toolchain/build-XXXXXX
}

#
# Common definitions
#

# Current list of platform levels we support
#
# Note: levels 6 and 7 are omitted since they have the same native
# APIs as level 5.
#
API_LEVELS="3 4 5 8 9"

# Location of the STLport sources, relative to the NDK root directory
STLPORT_SUBDIR=sources/cxx-stl/stlport

# Default ABIs for the prebuilt STLport binaries
STLPORT_ABIS="armeabi armeabi-v7a"

# Location of the GNU libstdc++ headers and libraries, relative to the NDK
# root directory.
GNUSTL_SUBDIR=sources/cxx-stl/gnu-libstdc++

# The date to use when downloading toolchain sources from android.git.kernel.org
# Leave it empty for tip of tree.
TOOLCHAIN_GIT_DATE=2010-12-13
