#!/bin/bash
set -e

source $ANDROID_BUILD_TOP/build/envsetup.sh
cd "$( dirname "${BASH_SOURCE[0]}" )"

ARCHES="arm armv5 arm64 mips mips64 x86 x86_64"
for arch in $ARCHES; do
  tapas all "$arch"
  mm -j40
done
