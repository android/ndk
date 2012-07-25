#!/bin/sh
#
# Copyright (C) 2010 The Android Open Source Project
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

# Script used to patch a source directory from a series of patches
# located under a directory hierarchy

. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS="<src-dir> <patches-dir>"
PROGRAM_DESCRIPTION=\
"Patch a target source directory with a series of patches taken
from another directory hierarchy. The idea is that anything that
is found under <patches-dir>/subdir/foo.patch will be applied with
'patch -p1' in <src-dir>/subdir.

Patches are applied in the order they are found by 'find'."

parse_parameters ()
{
    SRC_DIR=$1
    if [ -z "$SRC_DIR" ] ; then
        echo "ERROR: Missing source directory. See --help for usage."
        exit 1
    fi

    if [ ! -d "$SRC_DIR" ] ; then
        echo "ERROR: Invalid target source directory: $SRC_DIR"
        exit 1
    fi

    PATCHES_DIR=$2
    if [ -z "$PATCHES_DIR" ] ; then
        echo "ERROR: Missing patches directory. See --help for usage."
        exit 1
    fi

    if [ ! -d "$PATCHES_DIR" ] ; then
        echo "ERROR: Invalid patches directory: $PATCHES_DIR"
        exit 1
    fi
}

extract_parameters "$@"
parse_parameters $PARAMETERS

PATCHES=`(cd $PATCHES_DIR && find . -name "*.patch" | sort ) 2> /dev/null`
if [ -z "$PATCHES" ] ; then
    log "No patches files in $PATCHES_DIR"
    exit 0
fi
PATCHES=`echo $PATCHES | sed -e s%^\./%%g`
for PATCH in $PATCHES; do
    PATCHDIR=`dirname $PATCH`
    PATCHNAME=`basename $PATCH`
    log "Applying $PATCHNAME into $SRC_DIR/$PATCHDIR"
    cd $SRC_DIR/$PATCHDIR && patch -p1 < $PATCHES_DIR/$PATCH
    fail_panic "Patch failure with $PATCHES_DIR/$PATCH!! !! Please check your patches directory!"
done

dump "Done!"
