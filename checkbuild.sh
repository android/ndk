#!/bin/bash
set -e
set -x

cd "$( dirname "${BASH_SOURCE[0]}" )"

PACKAGE_DIR_ARG=
if [ -n "$DIST_DIR" ]; then
    PACKAGE_DIR_ARG="--package-dir=$DIST_DIR"
fi

build/tools/dev-cleanup.sh
build/tools/rebuild-all-prebuilt.sh \
    $(./realpath ../toolchain) \
    --verbose \
    $PACKAGE_DIR_ARG \
    $*
