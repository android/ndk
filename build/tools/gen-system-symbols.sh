#!/bin/sh
#
# Copyright (C) 2011 The Android Open Source Project
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
# gen-system-symbols.sh
#
# This tool is used to read the shared library from a source directory
# (SRC) and extract the list of functions and variables.
#
# Then, for each library, it will generate in (DST) two text files
# named <library>.functions.txt and <library>.variables.txt
#

# Only runs on Linux because it requires the "readelf" utility
#

. `dirname $0`/prebuilt-common.sh

VERBOSE=no
VERBOSE2=no

PROGRAM_PARAMETERS="<src-dir> <dst-dir>"
PROGRAM_DESCRIPTION=\
"This program is used to parse all shared libraries in <src-dir>
and extract, for each one of them, the list of functions and variables
that it exports.

For some of these libraries, it will remove symbols that are not meant
to be imported (unless you use --no-symbol-filtering)

These lists will then be saved into two files:

  <dst-dir>/<libname>.functions.txt
  <dst-dir>/<libname>.variables.txt
"

NO_FILTERING=
register_var_option "--no-symbol-filtering" NO_FILTERING "Disable symbol filtering"

extract_parameters "$@"

parse_params ()
{
    SRCDIR=$1
    DSTDIR=$2

    if [ -z "$SRCDIR" ]; then
        dump "ERROR: Missing first parameter (source directory path), see --help"
        exit 1
    fi

    if [ -z "$DSTDIR" ]; then
        dump "ERROR: Missing second parameter (destination directory path), see --help"
        exit 1
    fi

    if [ ! -d "$SRCDIR" ]; then
        dump "ERROR: Not a source directory: $SRCDIR"
        exit 1
    fi

    mkdir -p $DSTDIR
    fail_panic "Could not create destination directory: $DSTDIR"
}

parse_params $PARAMETERS

READELF=readelf

# $1: shared library path
get_library_functions ()
{
    $READELF -s -D -W $1 | awk '$5 ~ /FUNC/ && $6 ~ /GLOBAL|WEAK/ && $8 !~ /UND/ { print $9; }' | sort -u
}

# $1: shared library path
get_library_variables ()
{
    $READELF -s -D -W $1 | awk '$5 ~ /OBJECT/ && $6 ~ /GLOBAL|WEAK/ && $8 !~ /UND/ { print $9; }' | sort -u
}

# Temp file used to list shared library symbol exclusions
# See set_symbol_excludes and extract_shared_library_xxxx functions below
SYMBOL_EXCLUDES=/tmp/ndk-$USER/ndk-symbol-excludes.txt

# Temp file used to list shared library symbol inclusions, these
# are essentially overrides to the content of SYMBOL_EXCLUDES
SYMBOL_INCLUDES=/tmp/ndk-$USER/ndk-symbol-includes.txt

# Temp file used to filter symbols
SYMBOL_TMPFILE=/tmp/ndk-$USER/ndk-symbols-list.txt

# Reset the symbol exclusion list to its default
reset_symbol_excludes ()
{
    # By default, do not export C++ mangled symbol, which all start with _Z
    echo '^_Z' > $SYMBOL_EXCLUDES

    # __INIT_ARRAY__ and __FINI_ARRAY__ are special symbols that should
    # normally be hidden.
    echo "^__INIT_ARRAY__" >> $SYMBOL_EXCLUDES
    echo "^__FINI_ARRAY__" >> $SYMBOL_EXCLUDES
    > $SYMBOL_INCLUDES
}

# Add new exclusion patterns to SYMBOL_EXCLUDES
set_symbol_excludes ()
{
    (echo "$@" | tr ' ' '\n') >> $SYMBOL_EXCLUDES
}

# Add new inclusion patterns to SYMBOL_INCLUDES
set_symbol_includes ()
{
    (echo "$@" | tr ' ' '\n') >> $SYMBOL_INCLUDES
}

# Clear symbol exclusion/inclusion files
clear_symbol_excludes ()
{
    rm -f $SYMBOL_EXCLUDES $SYMBOL_INCLUDES
}

# Filter the list of symbols from a file
# $1: path to symbol list file
filter_symbols ()
{
    (grep -v -f $SYMBOL_EXCLUDES $1 ; grep -f $SYMBOL_INCLUDES $1) | sort -u
}

# $1: Library name
# $2+: List of symbols (functions or variables)
# Out: sorted list of filtered symbols, based on library name
filter_library_symbols ()
{
    local LIB=$1
    shift
    local SYMBOLS="$@"
    (echo "$SYMBOLS" | tr ' ' '\n' | sort -u) > $SYMBOL_TMPFILE

    reset_symbol_excludes

    case $LIB in
        libc.so)
            # Remove a few internal symbols that should not be exposed
            # from the C library (we plan to clean that up soon by using the
            # "hidden" visibility attribute in the near future).
            #
            set_symbol_excludes \
                '^the_' '^dns_' 'load_domain_search_list' 'res_get_dns_changed' \
                '^_resolv_cache' '^_dns_getht' '^_thread_atexit' \
                '^free_malloc_leak_info' 'fake_gmtime_r' 'fake_localtime_r' \
                '^gAllocationsMutex' '^gHashTable' '^gMallocLeakZygoteChild'
            ;;
        libstdc++.so)
            # This is the only library that is allowed to export C++ symbols for now.
            set_symbol_includes '^_Z.*'
            ;;
        liblog.so)
            set_symbol_excludes '^.*'         # exclude everything
            set_symbol_includes '^__android_' # except __android_xxxx functions
            ;;
        libOpenSLES.so)
            set_symbol_excludes '^_' '^MPH_' # remove MPH_to_xxx definitions
            ;;
        libGLESv*.so)
            # Exclude non-OES extension entry points
            set_symbol_excludes 'EXT$'
            set_symbol_excludes 'AMD$'
            set_symbol_excludes 'ANGLE$'
            set_symbol_excludes 'APPLE$'
            set_symbol_excludes 'IMG$'
            set_symbol_excludes 'NV$'
            set_symbol_excludes 'QCOM$'
            ;;
    esac
    filter_symbols "$SYMBOL_TMPFILE"
}

for LIB in $(cd $SRCDIR && ls lib*.so); do
    SRCLIB=$SRCDIR/$LIB
    log "Extracting symbols from $LIB"
    FUNCS=$(get_library_functions $SRCLIB)
    VARS=$(get_library_variables $SRCLIB)
    if [ -z "$NO_FILTERING" ]; then
        FUNCS=$(filter_library_symbols $LIB $FUNCS)
        VARS=$(filter_library_symbols $LIB $VARS)
    fi
    NUMFUNCS=$(echo $FUNCS | wc -w)
    NUMVARS=$(echo $VARS | wc -w)
    log "    Found $NUMFUNCS functions and $NUMVARS variables"
    (echo "$FUNCS" | tr ' ' '\n') > $DSTDIR/$LIB.functions.txt
    (echo "$VARS" | tr ' ' '\n') > $DSTDIR/$LIB.variables.txt
done
