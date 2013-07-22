# This script is used to check all pre-defined built-in macros in a given
# standalone toolchain. Call from tests/standalone/run.sh only.
#

macro_assign () {
    local _VARNAME=$1
    local _VARVALUE="$2"
    eval macro_$_VARNAME=\"$_VARVALUE\"
}

macro_val () {
    eval echo -n \"\$macro_$1\"
}

# Read all the built-in macros, and assign them to our own variables.
# For cygwin/mingw, don't use $NULL defined in parent run.sh to NUL, because
# NUL can't be used as input.  The non-existance /dev/null works well.
MACRO_LINES=$($CC $CFLAGS -dM -E - < /dev/null | sort -u | tr ' ' '^^^' | tr '"' '~')

for LINE in $MACRO_LINES; do
    # for cygwin, it's important to remove trailing '\r' as well
    LINE=$(echo "$LINE" | tr '^^^' ' ' | tr '\r' ' ')
    VARNAME=$(echo "$LINE" | cut -d' ' -f 2)
    VARVALUE=$(echo "$LINE" | cut -d' ' -f 3)

    # Avoid macro names that contain parentheses.
    echo "$VARNAME" | grep -q -v -e '('
    if [ $? != 0 ]; then
        continue
    fi

    macro_assign $VARNAME $VARVALUE
done

# Now perform some checks

FAILURES=0
COUNT=0

# $1: variable name
# $2: expected value
macro_expect () {

    local VAL=$(macro_val $1)
    if [ -z "$VAL" ]; then
        echo "Missing built-in macro definition: $1"
        return 1
    fi
    if [ "$VAL" != "$2" ]; then
        echo "Invalid built-in macro definition: '$VAL', expected '$2'"
        return 1
    fi
    return 0
}

# Check the definition of a given macro
# $1: macro name
# $2: expected value
# $3: textual description for the check
macro_check () {
    if [ -n "$3" ]; then
        echo -n "Checking $1 ($3): "
    else
        echo -n "Checking $1: "
    fi
    macro_expect "$1" "$2"
    if [ $? != 0 ]; then
        FAILURES=$(( $FAILURES + 1 ))
    else
        echo "ok"
    fi
    COUNT=$(( $COUNT + 1 ))
}

# Check the definition of a given macro against multiple values
# $1: macro name
# $2+: list of acceptable values.
macro_multi_check () {
    echo -n "Checking $1: "
    local VAL=$(macro_val $1)
    if [ -z "$VAL" ]; then
      echo "Missing built-in macro definition: $1"
      return 1
    fi
    local VAL2 FOUND
    shift
    for VAL2 in "$@"; do
      if [ "$VAL2" = "$VAL" ]; then
        FOUND=true
        break
      fi
    done
    if [ -z "$FOUND" ]; then
      echo "Invalid built-in macro definition: '$VAL', expected one of: $@"
      return 1
    fi
    return 0
}

# Check that a given macro is undefined
macro_check_undef () {
    if [ -n "$2" ]; then
        echo -n "Checking undefined $1 ($2): "
    else
        echo -n "Checking undefined $1: "
    fi
    local VAL="$(macro_val $1)"
    if [ -n "$VAL" ]; then
        echo "KO: Unexpected value '$VAL' encounteded"
        FAILURES=$(( $FAILURES + 1 ))
    else
        echo "ok"
    fi
    COUNT=$(( $COUNT + 1 ))
}

echo "Checking built-in macros for: $CC $CFLAGS"

# All toolchains must define the following prebuilt macros.
macro_check __ANDROID__ 1   "Android target system"
macro_check __linux__ 1     "Linux target system"
macro_check __unix__ 1      "Unix target system"
macro_check __ELF__ 1       "ELF target system"

# Either __pic__ or __PIC__ must be defined. Defining both is ok, not
# having anyone of them defined is an error.
#
# The value should be 1 on all platforms, except x86 where it will be 2
# (No idea why).
case $ABI in
    x86) PICVAL=2;;
    *) PICVAL=1;;
esac

case $ABI in
    armeabi|armeabi-v7a)
        macro_check __arm__ 1              "ARM CPU architecture"
        macro_check __ARM_EABI__ 1         "ARM EABI runtime"
        macro_check __ARMEL__ 1            "ARM little-endian"
        macro_check __THUMB_INTERWORK__ 1  "ARM thumb-interwork"
        macro_check __PIC__ 1              "Position independent code (-fpic)"
        macro_check __WCHAR_TYPE__         "unsigned"
        macro_check __WCHAR_MAX__          "4294967295U"
        # Clang doesn't define __WCHAR_MIN__ so don't check it"

        case $ABI in
            armeabi)
                macro_check __ARM_ARCH_5TE__ 1   "ARMv5TE instructions (for armeabi)"
                macro_check __SOFTFP__ 1         "ARM Soft-floating point"
                ;;
            armeabi-v7a)
                macro_check __ARM_ARCH_7A__ 1    "ARMv7-A instructions (for armeabi-v7a)"

                # This macro seems to be ill-named. It is only defined when we
                # don't use -mfloat-abi=softfp or -mfloat-abi=hard. I can only
                # assume it corresponds to -mfloat-abi=soft, which corresponds
                # to all FP operations implemented (slowly) through software.
                #
                # Not to be confused with -mfloat-abi=softfp which indicates
                # that the FPU is used for all FP operations, but that FP
                # values are passsed in core registers between function calls,
                # which is mandated by the armeabi-v7a definition.
                #
                macro_check_undef __SOFTFP__ 1   "ARM soft-floating point"
                ;;
        esac
        ;;

    x86)
        macro_check __i386__ 1       "x86 CPU architecture"
        macro_check __i686__ 1       "i686 instruction set"
        macro_check __PIC__ 2        "Position independent code (-fPIC)"
        macro_check __MMX__  1       "MMX instruction set"
        macro_check __SSE__ 1        "SSE instruction set"
        macro_check __SSE2__ 1       "SSE2 instruction set"
        macro_check __SSE3__ 1       "SSE3 instruction set"
        macro_check __SSE_MATH__ 1   "Use SSE for math operations"
        macro_check __SSE2_MATH__ 1  "Use SSE2 for math operations"
        # GCC defines is as 'long', and Clang as 'int'
        macro_multi_check __WCHAR_TYPE__   "long" "int"
        # GCC defines it with an L suffix, Clang doesn't.
        macro_multi_check __WCHAR_MAX__    "2147483647L" "2147483647"
        ;;

    mips)
        macro_check __mips__ 1          "Mips CPU architecture"
        macro_check _MIPS_ARCH_MIPS32 1 "Mips 32-bit ABI"
        macro_check __MIPSEL__ 1        "Mips little-endian"
        macro_check __PIC__ 1           "Position independent code (-fpic)"
        # GCC defines it as "signed int", and Clang as "int"
        macro_multi_check __WCHAR_TYPE__   "signed int" "int"
        macro_check __WCHAR_MAX__       "2147483647"
        ;;
    *)
        echo "Unknown ABI: $ABI"
        exit 1
esac

macro_check "__SIZEOF_SHORT__"       "2"   "short is 16-bit"
macro_check "__SIZEOF_INT__"         "4"   "int is 32-bit"
macro_check "__SIZEOF_FLOAT__"       "4"   "float is 32-bit"
macro_check "__SIZEOF_DOUBLE__"      "8"   "double is 64-bit"
macro_check "__SIZEOF_LONG_DOUBLE__" "8"   "long double is 64-bit"
macro_check "__SIZEOF_LONG_LONG__"   "8"   "long long is 64-bit"
macro_check "__SIZEOF_POINTER__"     "4"   "pointers are 32-bit"
macro_check "__SIZEOF_WCHAR_T__"     "4"   "wchar_t is 32-bit"

if [ "$FAILURES" = 0 ]; then
    echo "$COUNT/$COUNT tests passed. Nice job."
    exit 0
fi

echo "$FAILURES/$COUNT tests failed !!"
exit 1
