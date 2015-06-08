#!/bin/bash
set -e

cd ndk
TOOLCHAIN_DIR=$(realpath $(mktemp -d))

function cleanup {
  rm -rf $TOOLCHAIN_DIR
}
trap cleanup EXIT

build/tools/dev-cleanup.sh
build/tools/download-toolchain-sources.sh $TOOLCHAIN_DIR
build/tools/rebuild-all-prebuilt.sh $TOOLCHAIN_DIR --verbose $*
