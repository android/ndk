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
# cleanup-headers.sh
#
# This tool is used when we want to generate NDK arch headers.
# It will populate $NDK_DIR/../development/ndk/platforms/android-*/arch-$NEW_ARCH by default.
#

PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`
. "$PROGDIR/prebuilt-common.sh"
NDK_DIR=$ANDROID_NDK_ROOT

DEV_DIR="$NDK_DIR/../development"
register_var_option "--development-dir=<path>" DEV_DIR "Specify where the development dir is"

PROGRAM_PARAMETERS="<old-arch-name> <new-arch-name>"
PROGRAM_DESCRIPTION=\
"This tool is used when we want to generate NDK arch headers.
It will populate $NDK_DIR/../development/ndk/platforms/android-*/arch-$NEW_ARCH by default."

extract_parameters "$@"

set_parameters() {
  OLD_ARCH=$1
  NEW_ARCH=$2
  DEV_NDK_PLATFORM_DIR=$DEV_DIR/ndk/platforms
}

set_parameters $PARAMETERS

if [ -z "$OLD_ARCH" ] || [ -z "$NEW_ARCH" ]; then
  dump "[Error] Must specify old and new arch names"
  exit 1
fi

if [ -n "`find $DEV_NDK_PLATFORM_DIR -type d | grep arch-$NEW_ARCH`" ]; then
  log "[Warning] Already exist new arch headers. Remove it first."
  for API in $API_LEVELS; do
    rm -rf $DEV_NDK_PLATFORM_DIR/android-$API/arch-$NEW_ARCH
  done
fi

replace() {
  local HEADERS_DIR=$1
  local OLD=$2
  local NEW=$3
  FILES="`find $HEADERS_DIR | xargs grep $OLD | awk -F ':' '{print $1}' | sort -u | xargs`"
  for FILE in $FILES; do
    TMP_FILE=`mktemp`
    cp -a $FILE $TMP_FILE
    cat $TMP_FILE | sed -e "s/$OLD/$NEW/g" > $FILE
    rm -f $TMP_FILE
  done
}

remove() {
  local HEADERS_DIR=$1
  local PATTERN=$2
  local NAME=$3
  FILES="`find $HEADERS_DIR | xargs grep $NAME | awk -F ':' '{print $1}' | sort -u | xargs`"
  for FILE in $FILES; do
    TMP_FILE=`mktemp`
    cp -a $FILE $TMP_FILE
    cat $TMP_FILE | sed -e "s/.*${PATTERN}.*${NAME}.*//g" > $FILE
    rm -f $TMP_FILE
  done
}

OLD_PATTERN=ARM
NEW_PATTERN=MACHINE
TMP_PATTERN=NDKTMPMACHINE
KEEP_NO_RENAME_STUB=
replace_stub_to_temp() {
  local COMMAND=$1
  if [ "$COMMAND" = "add" ]; then
    local NAME=$2
    KEEP_NO_RENAME_STUB="$KEEP_NO_RENAME_STUB $NAME"
  elif [ "$COMMAND" = "do" ] || [ "$COMMAND" = "revert" ]; then
    local DIR=$2
    for stub in $KEEP_NO_RENAME_STUB; do
      local new_stub=`echo $stub | sed -e "s/$OLD_PATTERN/$TMP_PATTERN/g"`
      if [ "$COMMAND" = "do" ]; then
        replace $DIR $stub $new_stub
      else
        replace $DIR $new_stub $stub
      fi
    done
  fi
}

for API in $API_LEVELS; do
  log2 "API: $API"
  OLD_PLATFORM_DIR=$DEV_NDK_PLATFORM_DIR/android-$API/arch-$OLD_ARCH
  NEW_PLATFORM_DIR=$DEV_NDK_PLATFORM_DIR/android-$API/arch-$NEW_ARCH
  if [ -d "$OLD_PLATFORM_DIR/symbols" ]; then
    run copy_directory $OLD_PLATFORM_DIR/symbols $NEW_PLATFORM_DIR/symbols
  fi
  if [ -d "$OLD_PLATFORM_DIR/include" ]; then
    run copy_directory $OLD_PLATFORM_DIR/include $NEW_PLATFORM_DIR/include
  fi

  PATCH_PLATFORM_DIR=$DEV_NDK_PLATFORM_DIR/android-$API/header-patches
  if [ -d "$PATCH_PLATFORM_DIR" ]; then
    run rm -rf $NEW_PLATFORM_DIR/include/asm/arch
    for asm_header_stem in domain dyntick fpstate glue hardware ide mtd-xip pgtable-hwdef proc-fns procinfo suspend topology vga; do
      run rm -f $NEW_PLATFORM_DIR/include/asm/${asm_header_stem}.h
    done
    for machine_header_stem in cpu-features; do
      run rm -f $NEW_PLATFORM_DIR/include/machine/${machine_header_stem}.h
    done

    # Some keyword should not be replaced nor droped
    replace_stub_to_temp add ARMv6
    replace_stub_to_temp add PARMRK
    replace_stub_to_temp add __ARM_EABI__
    replace_stub_to_temp add __ARMEB__
    replace_stub_to_temp add __LINUX_ARM_ARCH__
    replace_stub_to_temp do $NEW_PLATFORM_DIR/include

    remove $NEW_PLATFORM_DIR/include "#include" glue.h
    remove $NEW_PLATFORM_DIR/include "#include" arch
    remove $NEW_PLATFORM_DIR/include "#include" mach
    remove $NEW_PLATFORM_DIR/include "#include" domain.h
    remove $NEW_PLATFORM_DIR/include "#include" pgtable-hwdef.h
    remove $NEW_PLATFORM_DIR/include "#include" proc-fns.h

    remove $NEW_PLATFORM_DIR/include "#define" EM_ARM
    remove $NEW_PLATFORM_DIR/include "#define" MODULE_ARCH_VERMAGIC
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_BASE
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_breakpoint
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_cacheflush
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_usr26
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_usr32
    remove $NEW_PLATFORM_DIR/include "#define" __ARM_NR_set_tls

    replace $NEW_PLATFORM_DIR $OLD_PATTERN $NEW_PATTERN
    replace_stub_to_temp revert $NEW_PLATFORM_DIR/include

    FILES="`cd $PATCH_PLATFORM_DIR && find include -type f | xargs`"
    for FILE in $FILES; do
      run mkdir -p $NEW_PLATFORM_DIR/`dirname $FILE`
      run cp -a $PATCH_PLATFORM_DIR/$FILE $NEW_PLATFORM_DIR/$FILE
    done

    PATCH_FILE=$PATCH_PLATFORM_DIR/headers.patch
    if [ -f "$PATCH_FILE" ]; then
      cd $NEW_PLATFORM_DIR && patch -p1 -s < $PATCH_FILE
    fi
  fi
done
