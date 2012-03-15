cd `dirname $0`
PWD=$(pwd)

# Update NDK_MODULE_PATH so we can find our imported modules
export NDK_MODULE_PATH="$PWD"

# Build everything
$NDK/ndk-build "$@"

# Extract ABIs list from parameters, we're looking for something like APP_ABI=<something>
PARAM_ABIS=$(echo "$@" | tr ' ' '\n' | grep -e "^APP_ABI=")
PARAM_ABIS=${PARAM_ABIS##APP_ABI=}
if [ -z "$PARAM_ABIS" ]; then
    echo "NO ABIS in param '$@'"
    ABIS="armeabi armeabi-v7a x86 mips"
else
    echo "FOUND ABIS in param '$@': $PARAM_ABIS"
    ABIS="$PARAM_ABIS"
fi

# Now ensure that all files were installed to all supported ABIs
MISSING=
for ABI in $ABIS; do
    DIR=$PWD/libs/$ABI
    for FILENAME in libfoo.so libpath1.so libpath2.so; do
        FILE=$DIR/$FILENAME
        if [ ! -f "$FILE" ]; then
            MISSING="$MISSING $FILE"
	fi
    done
done

# In case of missing files, error out
if [ "$MISSING" ]; then
    echo "ERROR: Missing files in build tree:"
    for FILE in $MISSING; do echo "  $FILE"; done
    exit 1
fi

# Otherwise, our test is good
exit 0
