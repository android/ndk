# Special script used to check that LOCAL_SHORT_COMMANDS works
# correctly even when using a very large number of source files
# when building a static or shared library.
#
# We're going to auto-generate all the files we need in a
# temporary directory, because that's how we roll.
#

PROGDIR=$(dirname $0)
PROGDIR=$(cd "$PROGDIR" && pwd)

# TODO: Increment this to 1000 for long test runs. For the simple unit test
#        suite, keep this just over 100
#
COUNT=110

# Generate all our source files
NUM=0

SRCDIR=$PROGDIR/jni
rm -rf "$SRCDIR" && mkdir -p "$SRCDIR"
if [ $? != 0 ]; then
    echo "ERROR: Could not create temporary source directory: $SRCDIR"
    exit 1
fi

CLEAN_FILES=

generate_source_files ()
{
    # Generate all temporary source files we need
    local NUM=0
    while [ $NUM -lt $COUNT ]; do
        SRCFILE=$SRCDIR/foo$NUM.c
    cat > $SRCFILE <<EOF
int foo$NUM (int x)
{
    return x + 1;
}
EOF
        NUM=$(( $NUM + 1 ))
        CLEAN_FILES=$CLEAN_FILES" $SRCFILE"
    done
}

generate_main_file ()
{
    cat > $SRCDIR/main.c <<EOF
#include <stdio.h>
EOF
    NUM=0
    while [ $NUM -lt $COUNT ]; do
        cat >> $SRCDIR/main.c <<EOF
extern int foo$NUM (int);
EOF
        NUM=$(( $NUM + 1 ))
    done
    cat >> $SRCDIR/main.c <<EOF
int main(void)
{
    int x = 0;
EOF
    NUM=0
    while [ $NUM -lt $COUNT ]; do
        cat >> $SRCDIR/main.c <<EOF
    x = foo$NUM(x);
EOF
        NUM=$(( $NUM + 1 ))
    done
cat >> $SRCDIR/main.c <<EOF
    return 0;
}
EOF
    CLEAN_FILES=$CLEAN_FILES" $SRCDIR/main.c"
}

generate_build_file ()
{
    local NUM

    # Generate the Android.mk
    cat > $SRCDIR/Android.mk <<EOF
# Auto-generated - do not edit
LOCAL_PATH := \$(call my-dir)
EOF

    # First, build a huge static library with all the files
    cat >> $SRCDIR/Android.mk <<EOF
include \$(CLEAR_VARS)
LOCAL_MODULE := libfoo_big
EOF

    NUM=0
    while [ $NUM -lt $COUNT ]; do
        cat >> $SRCDIR/Android.mk <<EOF
LOCAL_SRC_FILES += foo$NUM.c
EOF
        NUM=$(( $NUM + 1 ))
    done

    cat >> $SRCDIR/Android.mk <<EOF
LOCAL_SHORT_COMMANDS := true
include \$(BUILD_SHARED_LIBRARY)
EOF

    # Second, generate a large number of static libraries
    # Then an executable that use them all
    NUM=0
    while [ $NUM -lt $COUNT ]; do
        cat >> $SRCDIR/Android.mk <<EOF
include \$(CLEAR_VARS)
LOCAL_MODULE := foo$NUM
LOCAL_SRC_FILES := foo$NUM.c
include \$(BUILD_STATIC_LIBRARY)
EOF
        NUM=$(( $NUM + 1 ))
    done

        cat >> $SRCDIR/Android.mk <<EOF
include \$(CLEAR_VARS)
LOCAL_MODULE := test_linker_options_list
LOCAL_SRC_FILES := main.c
EOF

    NUM=0
    while [ $NUM -lt $COUNT ]; do
        cat >> $SRCDIR/Android.mk <<EOF
LOCAL_WHOLE_STATIC_LIBRARIES += foo$NUM
EOF
        NUM=$(( $NUM + 1 ))
    done

    cat >> $SRCDIR/Android.mk <<EOF
#LOCAL_SHORT_COMMANDS := true
include \$(BUILD_EXECUTABLE)
EOF
    CLEAN_FILES=$CLEAN_FILES" $SRCDIR/Android.mk"
}

generate_source_files && 
generate_main_file &&
generate_build_file
if [ $? != 0 ]; then
    echo "ERROR: Could not generate files for this test!"
    exit 1
fi

# Now run the build
$NDK/ndk-build -C "$PROGDIR" "$@"
RET=$?

# check if linker.list is empty
ALL_LINKER_LIST=`find obj -name linker.list`
for LINKER_LIST in $ALL_LINKER_LIST; do
    if [ ! -s $LINKER_LIST ]; then
        echo "ERROR: linker list file $LINKER_LIST isn't generated!"
        exit 1
    fi
done

# Clean everything we generated
rm -f $CLEAN_FILES
rm -rf "$PROGDIR/obj" "$PROGDIR/libs"
rm -rf jni

# Exit
exit $RET
