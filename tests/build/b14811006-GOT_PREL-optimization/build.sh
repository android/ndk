# Check if ARM compiler optimize GOT using GOT_PREL as much
# as possible, and turn instruction sequence of SkAlphaMulQ()
#
#	ldr	r3, .L2
#	ldr	r2, .L2+4
#   .LPIC0:
#	add	r3, pc, r3
#	ldr	r3, [r3, r2]
#	ldr	r3, [r3]
#	and	r2, r3, r0, lsr #8
#	....
#   .L2:
#	.word	_GLOBAL_OFFSET_TABLE_-(.LPIC0+8)
#	.word	gMask_00FF00FF(GOT)
#
# into
#
#	ldr	r2, .L2
#   .LPIC1:
#	ldr	r2, [pc, r2]
#	ldr	r3, [r2, #0]
#	and	r2, r3, r0, lsr #8
#	....
#   .L2:
#	.word	gMask_00FF00FF(GOT_PREL)+(.-(.LPIC1+8))
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

if [ -z "$APP_ABI" -o "$APP_ABI" = "all" ]; then
    APP_ABI="armeabi,armeabi-v7a"
fi

# Only test for armeabi and armeabi-v7a
if [ "$APP_ABI" != "${APP_ABI%%armeabi*}" ]; then
    APP_ABI=`echo $APP_ABI | tr ',' ' '`
    for ABI in $APP_ABI; do
        $NDK/ndk-build -B APP_ABI=$ABI APP_CFLAGS=-save-temps
        fail_panic "can't compile for APP_ABI=$ABI"
        fgrep -q "(GOT_PREL)" SkAlphaMulQ.s
        fail_panic "Fail to optimize GOT access with GOT_PREL, ABI=$ABI."
    done
fi

rm -rf libs obj SkAlphaMulQ.*