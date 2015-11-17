#!/bin/sh

. `dirname $0`/build/tools/ndk-common.sh

# Find all symlink in toolchains directory.
WIN_DIRS=`find toolchains`

dereference_symlink $WIN_DIRS

# The following should print nothing if we did good job
find toolchains/ -type l
