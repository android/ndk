#!/bin/sh
#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Run a few sanity checks on a given NDK release install/package

PROGNAME=$(basename "$0")
PROGDIR=$(dirname "$0")

. "$PROGDIR"/../build/tools/ndk-common.sh

panic () {
    echo "ERROR: $@" >&2
    exit 1
}

fail_panic () {
    if [ $? != 0 ]; then panic "$@"; fi
}

# Command-line processing. Please keep this alphabetically sorted.
HELP=
NDK_DIR=
NDK_PACKAGE=
SYSTEM=

for opt; do
    optarg=`expr "x$opt" : 'x[^=]*=\(.*\)'`
    case $opt in
    --help|-h|-?)
        HELP=true
        ;;
    --package=*)
        NDK_PACKAGE=$optarg
        ;;
    --system=*)
        SYSTEM=$optarg
        ;;
    -*)
        panic "Unknown option '$opt'. See --help for list of valid ones."
        ;;
    *)
        if [ -z "$NDK_DIR" ]; then
            NDK_DIR=$opt
        else
            panic "Only one parameter (ndk directory) is supported. See --help."
        fi
        ;;
    esac
done

if [ "$HELP" ]; then
    echo "Usage: $PROGNAME [options] [<ndk-install-path>]"
    echo ""
    echo "This script is used to run a series of sanity checks on a given"
    echo "NDK release installation, or alternatively an NDK release package."
    echo ""
    echo "Valid options:"
    echo ""
    echo "  --help|-h|-?      Print this message."
    echo "  --package=<file>  Specify NDK release archive file."
    echo "  --system=<name>   Specify host system type."
    echo ""
    exit 0
fi

if [ -n "$NDK_PACKAGE" ]; then
    if [ -n "$NDK_DIR" ]; then
        panic "You can't use --package=<file> and a directory path at the same time."
    fi
    TMP_DIR=/tmp/ndk-$USER/tests/release
    mkdir -p $TMP_DIR && rm -rf $TMP_DIR/*

    echo "Unpacking '$(basename $NDK_PACKAGE)' into: $TMP_DIR"
    unpack_archive "$NDK_PACKAGE" "$TMP_DIR"
    fail_panic "Could not uncompress NDK release package!"

    # Get into the first sub-directory. It should be something like android-ndk-*
    NDK_DIR=$(ls -d $TMP_DIR/android-ndk-* | head -1)
    if [ -z "$NDK_DIR" ]; then
        panic "Could not find uncompressed NDK directory. Please check your package file: $TMP_DIR"
    fi
elif [ -z "$NDK_DIR" ]; then
    panic "Please specify an NDK installation directory, or use --package=<file> option. See --help."
fi

FAILURES=0
COUNT=0

# Run a single test, and update failure/count appropriately.
# $1: test function name, without the test_ prefix.
# $2+: test textual abstract description.
do_test () {
    local NAME TEXT RET
    NAME=$1
    shift
    echo -n "test: $@... "
    TEXT=$(eval test_$NAME 2>/dev/null)
    if [ $? != 0 ]; then
        FAILURES=$(( $FAILURES + 1 ))
        echo "KO: $TEXT"
    else
        echo "ok"
    fi
    COUNT=$(( $COUNT + 1 ))
}

# There must be a RELEASE file at the top of the NDK
# installation tree.
#
test_RELEASE () {
    local RELEASE

    # Check that the RELEASE.TXT file is here
    if [ ! -f "$NDK_DIR/RELEASE.TXT" ]; then
        echo "Missing RELEASE.TXT file."
        return 1
    fi

    # Extract the release version
    RELEASE=$(cat $NDK_DIR/RELEASE.TXT)

    # Check that the ChangeLog file documents the release properly
    CHANGELOG_RELEASE=$(cat $NDK_DIR/docs/CHANGES.html | grep -e "^android-ndk-" | head -1)
    CHANGELOG_RELEASE=${CHANGELOG_RELEASE##android-ndk-}

    if [ "$RELEASE" != "$CHANGELOG_RELEASE" ]; then
        echo "CHANGES.html documents release '$CHANGELOG_RELEASE', but RELEASE.TXT contains '$RELEASE'!"
        return 1
    fi

    return 0
}


do_test RELEASE "Checking top-level release file"

echo -n "$FAILURES/$COUNT tests failed."
if [ "$FAILURES" = 0 ]; then
    echo "Congratulations!"
    exit 0
else
    echo "Please fix the errors!"
    exit 1
fi
