# Special script used to check that LOCAL_SHORT_COMMANDS works
# correctly even when using a very large number of source files
# when building a static or shared library.
#
# We're going to auto-generate all the files we need in a
# temporary directory, because that's how we roll.
#

PROGDIR=$(dirname $0)
PROGDIR=$(cd "$PROGDIR" && pwd)

# Clean generated binaries
rm -rf "$PROGDIR/obj" "$PROGDIR/libs"

# Now run the build
$NDK/ndk-build -C "$PROGDIR" "$@"
RET=$?

# check if linker.list is empty
ALL_SO=`find libs -name "*.so"`
for SO in $ALL_SO; do
    NUM_LINE=`objdump -s -j.rodata $SO | wc -l`
    if [ "$NUM_LINE" != "7" ]; then
        echo "ERROR: Fail to merge string literals!"
        exit 1
    fi
    NUM_ABCD=`objdump -s -j.rodata $SO | grep abcd | wc -l`
    if [ "$NUM_ABCD" != "2" ]; then
        echo "ERROR: abcd should appear exactly twice!"
        exit 1
    fi
done

# Clean generated binaries
rm -rf "$PROGDIR/obj" "$PROGDIR/libs"

# Exit
exit $RET
