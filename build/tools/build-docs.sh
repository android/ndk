#!/bin/sh
#
# Copyright (C) 2013 The Android Open Source Project
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

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

DEFAULT_OUT_DIR=/tmp/ndk-$USER/docs

IN_DIR=
OUT_DIR=$DEFAULT_OUT_DIR
FORCE=
RUN_CHECKS=

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION="Rebuild the HTML documentation from the Markdown text.

Rebuild the NDK html documentation from the Markdown input source files
in \$NDK/docs/text. See \$NDK/docs/tools/README for the input file format.

Builds are incremental, but you can use --force to rebuild everything.

Output files are placed in $DEFAULT_OUT_DIR, unless --out-dir=<path>
is used."

register_var_option "--force" FORCE "Rebuild all documentation"

register_var_option "--run-checks" RUN_CHECKS "Run internal consistency checks"

register_option "--in-dir=<path>" do_in_dir "Specify input directory" "\$NDK/docs/text"
do_in_dir () {
  IN_DIR=$1
}

register_var_option "--out-dir=<path>" OUT_DIR "Specify output directory"

MARKDOWN=markdown_py
register_var_option "--markdown=<executable>" MARKDOWN "Specify markdown executable to use"

extract_parameters "$@"

# Path to a Markdown filter program that is used to perform a few NDK-specific
# substitution in the input .text files. Note that this tool generates Markdown
# text, not HTML. TODO(digit): Use Markdown Extensions API to perform something
# similar, which is a lot more work though.
SUBST_PROGRAM=$ANDROID_NDK_ROOT/docs/tools/ndk-markdown-substitutions.py

if [ "$RUN_CHECKS" ]; then
  # Run unit tests for our $SUBST_PROGRAM
  $SUBST_PROGRAM --run-checks
  exit $?
fi

if [ -z "$IN_DIR" ]; then
  IN_DIR=$ANDROID_NDK_ROOT/docs/text
fi
IN_DIR=${IN_DIR%%/}
log "Input directory: $IN_DIR"

if [ -z "$OUT_DIR" ]; then
  OUT_DIR=$ANDROID_NDK_ROOT/docs
fi
OUT_DIR=${OUT_DIR%%/}
log "Output directory: $OUT_DIR"

TEMP_DIR=$NDK_TMPDIR/build-docs
mkdir -p "$TEMP_DIR"
fail_panic "Could not create temporary directory"

SRC_FILES=$(find "$IN_DIR" -name "*.text")
for SRC_FILE in $SRC_FILES; do
  # Compute destination file.
  DST_FILE=${SRC_FILE%%.text}.html
  DST_FILE=$OUT_DIR${DST_FILE##$IN_DIR}
  DST_DIR=$(dirname "$DST_FILE")
  # Only rebuild file is source is older than destination, unless --force
  # is being used.
  if [ -z "$FORCE" -a -f "$DST_FILE" -a "$SRC_FILE" -ot "$DST_FILE" ]; then
    log "Skipping: $SRC_FILE --> $DST_FILE"
  else
    log "Converting: $SRC_FILE --> $DST_FILE"
    mkdir -p "$DST_DIR"
    fail_panic "Could not create output directory: $DST_DIR"
    # Apply custom substitutions.
    TMP_FILE=$TEMP_DIR/$(basename "$DST_FILE").temp
    run $SUBST_PROGRAM --output "$TMP_FILE" "$SRC_FILE"
    fail_panic "Could not pre-process $SRC_FILE!"
    # Process with Markdown.
    run run $MARKDOWN --file="$DST_FILE" "$TMP_FILE"
    fail_panic "Could not convert $SRC_FILE!"
  fi
done

run rm -rf "$TEMP_DIR"

if [ "$OUT_DIR" != "$DEFAULT_OUT_DIR" ]; then
  dump "Done!"
else
  dump "Done, see $OUT_DIR"
fi

