cd $(dirname "$0")

# check if this test should be skipped

get_build_var ()
{
    if [ -z "$GNUMAKE" ] ; then
        GNUMAKE=make
    fi
    $GNUMAKE --no-print-dir -f $NDK/build/core/build-local.mk DUMP_$1 | tail -1
}

if [ -z "$APP_ABI" ] ; then
    # scan options for APP_ABI=
    for OPT in "$@"; do
        echo "$OPT" | grep -q -e "^APP_ABI="
        if [ $? = 0 ]; then
            APP_ABI=${OPT#APP_ABI=}
	fi
    done
fi

if [ -n "$APP_ABI" ] ; then
    # check APP_ABI
    APP_ABIS=`get_build_var APP_ABI`
echo "APP_ABIS=$APP_ABIS"
    APP_ABIS=$APP_ABIS" "
    if [ "$APP_ABIS" != "${APP_ABIS%%all*}" ] ; then
        # replace the first "all" with all available ABIs
        ALL_ABIS=`get_build_var $DIR NDK_ALL_ABIS`
        APP_ABIS_FRONT="${APP_ABIS%%all*}"
        APP_ABIS_BACK="${APP_ABIS#*all}"
        APP_ABIS="${APP_ABIS_FRONT}${ALL_ABIS}${APP_ABIS_BACK}"
    fi
    if [ "$APP_ABIS" = "${APP_ABIS%$APP_ABI *}" ] ; then
        echo "Skipping `basename $1`: incompatible ABI, needs $APP_ABIS"
        exit 0
    fi
fi

if [ -f "BROKEN_BUILD" ] ; then
    TESTPATH=`pwd`
    if [ ! -s "BROKEN_BUILD" ] ; then
        # skip all
        echo "Skipping `basename $TESTPATH`: (build)"
        exit 0
    else
        # only skip listed in file
        TARGET_TOOLCHAIN=`get_build_var TARGET_TOOLCHAIN`
        TARGET_TOOLCHAIN_VERSION=`echo $TARGET_TOOLCHAIN | tr '-' '\n' | tail -1`
        grep -q -w -e "$TARGET_TOOLCHAIN_VERSION" "BROKEN_BUILD"
        if [ $? = 0 ] ; then
            echo "Skipping `basename $TESTPATH`: (no build for $TARGET_TOOLCHAIN_VERSION)"
            exit 0
        fi
    fi
fi

$NDK/ndk-build -B "$@"

# Find instruction in file
# $1: instruction
# $2: file
find_instruction ()
{
    local INST=$1
    local FILE=$2

    grep -q -w -F -e $INST $FILE
    if [ $? != 0 ]; then
        echo "$INST expected in file $FILE"
        exit 1
    fi
}

find_instruction "madd.d" mips-fp4-test1-2.s
find_instruction "msub.d" mips-fp4-test1-2.s
find_instruction "nmadd.d" mips-fp4-test3-6.s
find_instruction "nmsub.d" mips-fp4-test3-6.s
find_instruction "recip.d" mips-fp4-test3-6.s
find_instruction "rsqrt.d" mips-fp4-test3-6.s

rm -f *.s *.i
