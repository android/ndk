#!/bin/sh
#
# Copyright (C) 2011 The Android Open Source Project
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
# dev-platform-compress.sh
#
# Compressed expanded platform files into development/ndk/platforms/
# structure.
#

PROGDIR=$(dirname $0)
. $PROGDIR/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION=\
"This script is used to compress an expanded platforms NDK tree
into the compressed/minimal structure used in development/ndk/platforms.

The main idea is that in the destination directory, a file only appears
once, even if it is provided by several platforms. I.e. if we have:

  file1 = \$SRC/android-3/foo
  file2 = \$SRC/android-4/foo
  file3 = \$SRC/android-5/foo

We will always store a copy of file1 under \$DST/android-3/foo
If file2 is identical to file1, we remove its copy in \$DST/android-4/foo,
otherwise we do copy it to the same location

If file3 is identical to file2, we remove its copy in \$DST/android-4/foo,
otherwise we copy it to the same location.

Repeat for all files under \$SRC/android-N for increasing values of N.
"

SRCDIR=/tmp/ndk-$USER/platforms
register_var_option "--src-dir=<path>" SRCDIR "Specify source platforms directory"

DSTDIR=/tmp/ndk-$USER/platforms-compressed
register_var_option "--dst-dir=<path>" DSTDIR "Specify destination directory"

API_LEVELS=$(spaces_to_commas $API_LEVELS)
register_var_option "--platforms=<list>" API_LEVELS "Specify all API levels"

extract_parameters "$@"

API_LEVELS=$(commas_to_spaces $API_LEVELS)

# Sanity check
for PLATFORM in $API_LEVELS; do
    SDIR=$SRCDIR/android-$PLATFORM
    if [ ! -d "$SDIR" ]; then
        echo "ERROR: Missing source platform directory: $SDIR"
        exit 1
    fi
done

# Let's roll
PREV_PLATFORM=
for PLATFORM in $API_LEVELS; do
    SDIR=$SRCDIR/android-$PLATFORM
    DDIR=$DSTDIR/android-$PLATFORM
    if [ -z "$PREV_PLATFORM" ]; then
        # Copy everything here
        log "Copying directory: $SDIR --> $DDIR"
        copy_directory "$SDIR" "$DDIR"
    else
        # For each file, check whether it is new or
        # different from the one in the previous platform level
        log "Compressing directory: $SDIR"
        PDIR=$SRCDIR/android-$PREV_PLATFORM
        FILES=$(cd $SDIR && find . -type f)
        if [ "$VERBOSE2" = "yes" ]; then
            echo "Files found:"
            echo "$FILES" | tr ' ' '\n'
        fi
        for FILENAME in $FILES; do
            FILENAME=${FILENAME##./}  # Get rid of leading ./
            PFILE=$PDIR/$FILENAME
            CFILE=$SDIR/$FILENAME
            DFILE=$DDIR/$FILENAME
            if [ -f "$PFILE" ]; then
                log2 "Comparing $CFILE with $PFILE"
                if cmp --quiet $PFILE $CFILE; then
                    # Files are identical, remove it from destination
                    # if it exists there, it's not longer relevant.
                    if [ -f "$DFILE" ]; then
                        log2 "Removing obsolete $DFILE"
                        rm -f $DFILE
                    else
                        log2 "Skipping $CFILE"
                    fi
                    continue
                fi
            fi
            # New or modified file, copy it
            DFILE=$DDIR/$FILENAME
            log2 "Copying $SFILE --> $DFILE"
            mkdir -p $(dirname "$DFILE") && cp $CFILE $DFILE
            fail_panic "Could not copy $CFILE to $DFILE"
        done
    fi
    PREV_PLATFORM=$PLATFORM
done

log "Done!"
exit 0
