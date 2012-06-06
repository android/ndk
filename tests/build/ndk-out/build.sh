cd $(dirname "$0")
rm -rf obj output-dir
export NDK_OUT=output-dir
$NDK/ndk-build "$@"
if [ -d obj ] ; then
    echo "FAILURE: 'obj' directory should not be created by ndk-build!"
    exit 1
fi
if [ ! -d output-dir ]; then
    echo "FAILURE: 'output-dir' should have been created by ndk-build!"
    exit 1
fi
rm -rf output-dir


