# Check if APP_ABI=armeabi-v7a use "rev" instructions for __swapXX
# See https://android-review.googlesource.com/#/c/17144
#

fail_panic ()
{
    if [ $? != 0 ] ; then
        echo "ERROR: $@"
        exit 1
    fi
}

for opt do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case "$opt" in
    APP_ABI=*)
        APP_ABI=$optarg
        ;;
    esac
done

if [ -z "$APP_ABI" -o "$APP_ABI" = "all" -o "$APP_ABI" != "${APP_ABI%%armeabi-v7a*}" ]; then
    # checking armeabi-v7a
    $NDK/ndk-build -B APP_ABI=armeabi-v7a APP_CFLAGS=-save-temps NDK_DEBUG=1
    fail_panic "can't compile for APP_ABI=armeabi-v7a"
    grep -qw rev16 issue17144-byteswap.s
    fail_panic "armeabi-v7a doesn't use rev16 instruction for __swap16()"
    grep -qw rev issue17144-byteswap.s
    fail_panic "armeabi-v7a doesn't use rev instruction for __swap32()"
fi

if [ -z "$APP_ABI" -o "$APP_ABI" = "all" -o "$APP_ABI" != "${APP_ABI%%x86}" ]; then
    # checking x86
    $NDK/ndk-build -B APP_ABI=x86 APP_CFLAGS=-save-temps NDK_DEBUG=1
    fail_panic "can't compile for x86"
    grep -qw 'ro[lr]w' issue17144-byteswap.s
    fail_panic "x86 doesn't use rorw instruction for __swap16()"
    egrep -qw 'bswapl?' issue17144-byteswap.s
    fail_panic "x86 doesn't use bswap instruction for __swap32()"
fi

if [ -z "$APP_ABI" -o "$APP_ABI" = "all" -o "$APP_ABI" != "${APP_ABI%%mips}" ]; then
    # checking mips
    # Note that MD_SWAP in machine/endian.h is only defined for r2.  Add
    # -mips32r2 because default Android toolchain support r1
    $NDK/ndk-build -B APP_ABI=mips APP_CFLAGS="-save-temps -mips32r2" NDK_DEBUG=1
    fail_panic "can't compile for mips"
    grep -qw wsbh issue17144-byteswap.s
    fail_panic "mips doesn't use wsbh instruction for __swap16()"
    grep -wA1 wsbh issue17144-byteswap.s | egrep -qw 'rot?r'
    fail_panic "mips doesn't use wsbh/rotr instruction for __swap32()"
fi

rm -rf libs obj issue17144-byteswap.*
