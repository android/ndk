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

$NDK/ndk-build -B APP_CFLAGS=-save-temps
fail_panic "can't compile!"
grep -w madd cortex-a53-835769-1.s | grep -qw w0,
fail_panic "fail to generate correct code for cortex-a53-835769-1.c"
grep -w madd cortex-a53-835769-2.s | grep -qw w0,
fail_panic "fail to generate correct code for cortex-a53-835769-2.c"

rm -rf libs obj cortex-a53-835769-*
