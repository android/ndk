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

if [ "$SYSTEM" = "linux-x86" -a -d "$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/windows" ] ; then
    # enumerate all cases using windows toolchain
    dump "### Running windows 4.7 full tests"
    NDK_TOOLCHAIN_VERSION=4.7 ./run-tests.sh --continue-on-build-fail --full --wine
    dump "### Running windows 4.6 full tests"
    NDK_TOOLCHAIN_VERSION=4.6 ./run-tests.sh --continue-on-build-fail --full --wine
    dump "### Running windows 4.4.3 full tests"
    NDK_TOOLCHAIN_VERSION=4.4.3 ./run-tests.sh --continue-on-build-fail --full --wine
    dump "### Running windows clang 3.1 full tests"
    NDK_TOOLCHAIN_VERSION=clang3.1 ./run-tests.sh --continue-on-build-fail --full --wine
fi

# add more if you want ...

#
# Run standalone tests
#
STANDALONE_TMPDIR=$NDK_TMPDIR

# $1: API level
# $2: Arch
# $3: GCC version
standalone_path ()
{
    local API=$1
    local ARCH=$2
    local GCC_VERSION=$3

    echo ${STANDALONE_TMPDIR}/android-ndk-api${API}-${ARCH}-${GCC_VERSION}
}

# $1: API level
# $2: Arch
# $3: GCC version
# $4: LLVM version
make_standalone ()
{
    local API=$1
    local ARCH=$2
    local GCC_VERSION=$3
    local LLVM_VERSION=$4

    (cd $NDK && \
     ./build/tools/make-standalone-toolchain.sh \
        --platform=android-$API \
        --install-dir=$(standalone_path $API $ARCH $GCC_VERSION) \
        --llvm-version=$LLVM_VERSION \
        --toolchain=$(get_toolchain_name_for_arch $ARCH $GCC_VERSION))
}

API=14
LLVM_VERSION=$DEFAULT_LLVM_VERSION
for ARCH in $(commas_to_spaces $DEFAULT_ARCHS); do
    for GCC_VERSION in $(commas_to_spaces $DEFAULT_GCC_VERSION_LIST); do
        dump "### Testing $ARCH gcc-$GCC_VERSION toolchain with --sysroot"
        (cd $NDK && \
            ./tests/standalone/run.sh --prefix=$(get_toolchain_binprefix_for_arch $ARCH $GCC_VERSION)-gcc)
        dump "### Making $ARCH gcc-$GCC_VERSION standalone toolchain"
        make_standalone $API $ARCH $GCC_VERSION $LLVM_VERSION
        dump "### Testing $ARCH gcc-$GCC_VERSION standalone toolchain"
        (cd $NDK && \
            ./tests/standalone/run.sh --no-sysroot \
                --prefix=$(standalone_path $API $ARCH $GCC_VERSION)/bin/$(get_default_toolchain_prefix_for_arch $ARCH)-gcc)
        dump "### Testing clang in $ARCH gcc-$GCC_VERSION standalone toolchain"
        (cd $NDK && \
            ./tests/standalone/run.sh --no-sysroot \
                --prefix=$(standalone_path $API $ARCH $GCC_VERSION)/bin/clang)
    done
done

# clean up
rm -rf $STANDALONE_TMPDIR
