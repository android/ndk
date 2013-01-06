#!/bin/sh

PROGDIR=`dirname $0`
NDK=`cd $PROGDIR/.. && pwd`
NDK_BUILDTOOLS_PATH=$NDK/build/tools
. $NDK/build/core/ndk-common.sh
. $NDK/build/tools/prebuilt-common.sh

# Find all devices
DEVICE_arm=
DEVICE_mips=
DEVICE_x86=

ADB_CMD=`which adb`
if [ -n $ADB_CMD ] ; then
    # Get list of online devices, turn ' ' in device into '.'
    DEVICES=`$ADB_CMD devices | grep -v offline | awk 'NR>1 {gsub(/[ \t]+device$/,""); print;}' | sed '/^$/d' | tr ' ' '.'`
    for DEVICE in $DEVICES; do
        # undo previous ' '-to-'.' translation
        DEVICE=$(echo $DEVICE | tr '.' ' ')
        # get arch
        ARCH=`$ADB_CMD -s "$DEVICE" shell getprop ro.product.cpu.abi | tr -dc '[:print:]'`
        case "$ARCH" in
            armeabi*)
                    DEVICE_arm=$DEVICE
                    ;;
            x86)
                    DEVICE_x86=$DEVICE
                    ;;
            mips)
                    DEVICE_mips=$DEVICE
                    ;;
            *)
                    echo "ERROR: Unsupported architecture: $ARCH"
                    exit 1
        esac
    done
fi

echo "DEVICE_arm=$DEVICE_arm"
echo "DEVICE_x86=$DEVICE_x86"
echo "DEVICE_mips=$DEVICE_mips"

#
# check if we need to also test 32-bit host toolchain
#
TEST_HOST_32BIT=no
TAGS=$HOST_TAG
case "$HOST_TAG" in
    linux-x86_64|darwin-x86_64)
        if [ -d "$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/$HOST_TAG" ] ; then
            # ideally we should check each individual compiler the presence of 64-bit
            # but for test script this is fine
            TEST_HOST_32BIT=yes
            TAGS=$TAGS" $HOST_TAG32"
        fi
    ;;
    windows*)
        if [ "$ProgramW6432"!="" -a \
             -d "$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/windows-x86_64" ] ; then
            TEST_HOST_32BIT=yes
            TAGS=$TAGS" windows-x86_64"
        fi
esac

#
# Run run-tests.sh
#
SYSTEM=$(get_prebuilt_host_tag)

# keep this simple, only intend to test the case when NDK_TOOLCHAIN_VERSION isn't specified
dump "### Run simple tests"
ANDROID_SERIAL=none ./run-tests.sh --continue-on-build-fail --abi=armeabi

# enumerate all cases using $SYSTEM toolchain
dump "### Running $SYSTEM gcc 4.7 full tests"
NDK_TOOLCHAIN_VERSION=4.7 ./run-tests.sh --continue-on-build-fail --full
dump "### Running $SYSTEM gcc 4.6 full tests"
NDK_TOOLCHAIN_VERSION=4.6 ./run-tests.sh --continue-on-build-fail --full
dump "### Running $SYSTEM gcc 4.4.3 full tests"
NDK_TOOLCHAIN_VERSION=4.4.3 ./run-tests.sh --continue-on-build-fail --full
dump "### Running $SYSTEM clang 3.1 full tests"
NDK_TOOLCHAIN_VERSION=clang3.1 ./run-tests.sh --continue-on-build-fail --full

if [ "$TEST_HOST_32BIT" = "yes" ] ; then
    dump "### Running $SYSTEM gcc 4.7 tests (32-bit host)"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.7 ./run-tests.sh --continue-on-build-fail
    dump "### Running $SYSTEM gcc 4.6 tests (32-bit host)"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.6 ./run-tests.sh --continue-on-build-fail
    dump "### Running $SYSTEM gcc 4.4.3 tests (32-bit host)"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.4.3 ./run-tests.sh --continue-on-build-fail
    dump "### Running $SYSTEM clang 3.1 tests (32-bit host)"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=clang3.1 ./run-tests.sh --continue-on-build-fail
fi

if [ "$SYSTEM" = "linux-x86" -a -d "$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/windows-x86_64" ] ; then
    # using 64-bit windows toolchain
    dump "### Running windows-x86_64 4.7 tests"
    NDK_TOOLCHAIN_VERSION=4.7 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows-x86_64 4.6 tests"
    NDK_TOOLCHAIN_VERSION=4.6 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows-x86_64 4.4.3 tests"
    NDK_TOOLCHAIN_VERSION=4.4.3 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows-x86_64 clang 3.1 tests"
    NDK_TOOLCHAIN_VERSION=clang3.1 ./run-tests.sh --continue-on-build-fail --wine # --full
fi

if [ "$SYSTEM" = "linux-x86" -a -d "$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/windows" ] ; then
    # using 32-bit windows toolchain
    dump "### Running windows 4.7 tests"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.7 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows 4.6 tests"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.6 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows 4.4.3 tests"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=4.4.3 ./run-tests.sh --continue-on-build-fail --wine # --full
    dump "### Running windows clang 3.1 tests"
    NDK_HOST_32BIT=1 NDK_TOOLCHAIN_VERSION=clang3.1 ./run-tests.sh --continue-on-build-fail --wine # --full
fi

# add more if you want ...

#
# Run standalone tests
#
STANDALONE_TMPDIR=$NDK_TMPDIR

# $1: Host tag
# $2: API level
# $3: Arch
# $4: GCC version
standalone_path ()
{
    local TAG=$1
    local API=$2
    local ARCH=$3
    local GCC_VERSION=$4

    echo ${STANDALONE_TMPDIR}/android-ndk-api${API}-${ARCH}-${TAG}-${GCC_VERSION}
}

# $1: Host tag
# $2: API level
# $3: Arch
# $4: GCC version
# $5: LLVM version
make_standalone ()
{
    local TAG=$1
    local API=$2
    local ARCH=$3
    local GCC_VERSION=$4
    local LLVM_VERSION=$5

    (cd $NDK && \
     ./build/tools/make-standalone-toolchain.sh \
        --platform=android-$API \
        --install-dir=$(standalone_path $TAG $API $ARCH $GCC_VERSION) \
        --llvm-version=$LLVM_VERSION \
        --toolchain=$(get_toolchain_name_for_arch $ARCH $GCC_VERSION) \
        --system=$TAG)
}

API=14
LLVM_VERSION=$DEFAULT_LLVM_VERSION
for ARCH in $(commas_to_spaces $DEFAULT_ARCHS); do
    for GCC_VERSION in $(commas_to_spaces $DEFAULT_GCC_VERSION_LIST); do
        for TAG in $TAGS; do
            dump "### [$TAG] Testing $ARCH gcc-$GCC_VERSION toolchain with --sysroot"
            (cd $NDK && \
                ./tests/standalone/run.sh --prefix=$(get_toolchain_binprefix_for_arch $ARCH $GCC_VERSION $TAG)-gcc)
            dump "### [$TAG] Making $ARCH gcc-$GCC_VERSION standalone toolchain"
            make_standalone $TAG $API $ARCH $GCC_VERSION $LLVM_VERSION
            dump "### [$TAG] Testing $ARCH gcc-$GCC_VERSION standalone toolchain"
            (cd $NDK && \
                ./tests/standalone/run.sh --no-sysroot \
                    --prefix=$(standalone_path $TAG $API $ARCH $GCC_VERSION)/bin/$(get_default_toolchain_prefix_for_arch $ARCH)-gcc)
            dump "### [$TAG] Testing clang in $ARCH gcc-$GCC_VERSION standalone toolchain"
            (cd $NDK && \
                ./tests/standalone/run.sh --no-sysroot \
                    --prefix=$(standalone_path $TAG $API $ARCH $GCC_VERSION)/bin/clang)
	    rm -rf $(standalone_path $TAG $API $ARCH $GCC_VERSION)
        done
    done
done

# clean up
rm -rf $STANDALONE_TMPDIR
