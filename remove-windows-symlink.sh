#!/bin/sh

. `dirname $0`/build/tools/ndk-common.sh

# Find all symlink in toolchains/windows and windows-x86_64 directories
WIN_DIRS=`find toolchains \( -name "windows" -o -name "windows-x86_64" \)`

dereference_symlink $WIN_DIRS

# The following should print nothing if we did good job
find toolchains/ -type l | grep windows
