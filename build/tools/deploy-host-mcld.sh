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
# Rebuild the host GCC toolchain binaries from sources.
#
# NOTE: this script does not rebuild gdb, see build-host-gdb.sh for this.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh

PROGRAM_PARAMETERS=""
PROGRAM_DESCRIPTION="\
This program is used to deploy mclinker (ld.mcld) to GCC directories.
Although ld.mcld depends on lots of LLVM modules and is built in
build-llvm.sh to reduce long LLVM compilation time, it can be used as
a drop-in replacement for ld.bfd and ld.gold in GCC.

Running after completion of both build-llvm.sh and build-[host-]gcc.sh,
this script copy toolchains/llvm-$DEFAULT_LLVM_VERSION/prebuilt/$SYSTEM/bin/ld.mcld[.exe]
to be sibling of ld in all GCC directories with same HOST_OS and bitness,
ie. {linux, darwin, windows} x {64, 32}

If --systems isn't specified, this script discovers all ld.mcld[.exe] in
toolchains/llvm-$DEFAULT_LLVM_VERSION

Note that one copy of ld.mcld serves all GCC {4.8, 4.7, 4.6, 4.4.3} x {arm, x86, mips}.
GCC passes -m flag for ld.mcld to figure out the right target.
"
NDK_DIR=
register_var_option "--ndk-dir=<path>" NDK_DIR "NDK installation directory"

PACKAGE_DIR=
register_var_option "--package-dir=<path>" PACKAGE_DIR "Create archive tarball in specific directory"

SYSTEMS=
register_var_option "--systems=<list>" SYSTEMS "List of host systems to deply ld.mcld for"

extract_parameters "$@"

if [ -z "$NDK_DIR" ] ; then
    NDK_DIR=$ANDROID_NDK_ROOT
    log "Auto-config: --ndk-dir=$NDK_DIR"
else
    if [ ! -d "$NDK_DIR" ] ; then
        echo "ERROR: NDK directory does not exists: $NDK_DIR"
        exit 1
    fi
fi

if [ "$PACKAGE_DIR" ]; then
    mkdir -p "$PACKAGE_DIR"
    fail_panic "Could not create package directory: $PACKAGE_DIR"
fi

cd $NDK_DIR

if [ -z "$SYSTEMS" ]; then
    # find all ld.mcld
    ALL_MCLDS=`find toolchains/llvm-$DEFAULT_LLVM_VERSION -name "ld.mcld*"`

    for MCLD in $ALL_MCLDS; do
        # compute SYSTEM of this ld.mcld
        SYSTEM=${MCLD%%/bin/*}
        SYSTEM=${SYSTEM##*prebuilt/}
        SYSTEMS=$SYSTEMS" $SYSTEM"
    done
fi

for SYSTEM in $SYSTEMS; do
    HOST_EXE=
    if [ "$SYSTEM" != "${SYSTEM%%windows*}" ] ; then
        HOST_EXE=.exe
    fi

    MCLD=toolchains/llvm-$DEFAULT_LLVM_VERSION/prebuilt/$SYSTEM/bin/ld.mcld$HOST_EXE
    test -f "$MCLD" || fail_panic "Could not find $MCLD"

    # find all GNU ld with the same SYSTEM
    ALL_LDS=`find toolchains \( -name "*-ld" -o -name "ld" -o -name "*-ld.exe" -o -name "ld.exe" \) | egrep "/arm|/x86|/mips" | grep $SYSTEM/`

    ALL_LD_MCLDS=
    for LD in $ALL_LDS; do
        LD_NOEXE=${LD%%.exe}
        LD_MCLD=${LD_NOEXE}.mcld$HOST_EXE
        run rm -f "$LD_MCLD"
        if [ "$LD_NOEXE" != "${LD_NOEXE%%/ld}" ] ; then
          # ld in $ABI/bin/ld
            run ln -s "../../../../../../$MCLD" "$LD_MCLD"
        else
          # ld in bin/$ABI-ld
            run ln -s "../../../../../$MCLD" "$LD_MCLD"
        fi
        ALL_LD_MCLDS=$ALL_LD_MCLDS" $LD_MCLD"
    done

    # package
    if [ "$PACKAGE_DIR" ]; then
        ARCHIVE="ld.mcld-$SYSTEM.tar.bz2"
        #echo $ARCHIVE
        echo  "Packaging $ARCHIVE"
        pack_archive "$PACKAGE_DIR/$ARCHIVE" "$NDK_DIR" $ALL_LD_MCLDS
    fi
done

dump "Done."
