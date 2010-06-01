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
# $2: name of function that will be called when the option is parsed
# $3: small abstract for the option
# $4: optional. default value
#
register_option ()
{
    local optname optvalue opttype optlabel
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
    option_set_attr $optname funcname "$2"
    option_set_attr $optname abstract "$3"
    option_set_attr $optname default "$4"
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
                value=
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
                value=
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
        # Launch option-specific function, value, if any as argument
        eval `option_get_attr $name funcname` \"$value\"
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

# Determine sysroot
# $1: Option value (or empty)
#
fix_sysroot ()
{
    if [ -n "$1" ] ; then
        eval SYSROOT="$1"
        log "Using specified sysroot: $1"
    else
        SYSROOT_SUFFIX=build/platforms/$PLATFORM/arch-$ARCH
        if [ -d $NDK_DIR/$SYSROOT_SUFFIX ] ; then
            SYSROOT=$NDK_DIR/$SYSROOT_SUFFIX
            log "Using target NDK sysroot: $SYSROOT"
        else
            SYSROOT=$ANDROID_NDK_ROOT/$SYSROOT_SUFFIX
            log "Using install NDK sysroot: $SYSROOT"
        fi
    fi

    if [ ! -f $SYSROOT/usr/include/stdlib.h ] ; then
        echo "ERROR: Invalid sysroot path: $SYSROOT"
        echo "       Use --sysroot=<path> to indicate a valid one."
        exit 1
    fi
}

prepare_host_flags ()
{
    # Force generation of 32-bit binaries on 64-bit systems
    case $HOST_TAG in
        *-x86_64)
            HOST_CFLAGS="$HOST_CFLAGS -m32"
            HOST_LDFLAGS="$HOST_LDFLAGS -m32"
            force_32bit_binaries  # to modify HOST_TAG and others
            ;;
    esac
}

parse_toolchain_name ()
{
    if [ -z "$TOOLCHAIN" ] ; then
        echo "ERROR: Missing toolchain name!"
        exit 1
    fi

    # Determine ABI based on toolchain name
    #
    case "$TOOLCHAIN" in
    arm-eabi-*)
        ARCH="arm"
        ABI_INSTALL_NAME="arm-eabi"
        ABI_TOOLCHAIN_PREFIX="arm-eabi"
        ABI_CONFIGURE_HOST="arm-eabi-linux"
        ;;
    x86-*)
        ARCH="x86"
        ABI_INSTALL_NAME="x86"
        ABI_TOOLCHAIN_PREFIX="i686-android-linux-gnu"
        ABI_CONFIGURE_HOST="i686-linux"
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

}

set_toolchain_install ()
{
    TOOLCHAIN_PATH=$1
    log "Using toolchain path: $TOOLCHAIN_PATH"

    TOOLCHAIN_PREFIX=$TOOLCHAIN_PATH/bin/$ABI_TOOLCHAIN_PREFIX
    log "Using toolchain prefix: $TOOLCHAIN_PREFIX"
}

check_toolchain_install ()
{
    TOOLCHAIN_PATH=$1/build/prebuilt/$HOST_TAG/$TOOLCHAIN
    if [ ! -d "$TOOLCHAIN_PATH" ] ; then
        echo "ERROR: Toolchain '$TOOLCHAIN' not installed in '$NDK_DIR'!"
        echo "       Ensure that the toolchain has been installed there before."
        exit 1
    fi

    set_toolchain_install $TOOLCHAIN_PATH
}


random_temp_directory ()
{
    mktemp -d /tmp/ndk-toolchain-XXXXXX
}
