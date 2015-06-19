#!/bin/bash
set -e

cd "$( dirname "${BASH_SOURCE[0]}" )"

build/tools/dev-cleanup.sh
build/tools/rebuild-all-prebuilt.sh $(./realpath ../toolchain) --verbose $*
