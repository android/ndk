#!/bin/sh
#
# Copyright (C) 2009 The Android Open Source Project
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
#  This shell script is used to download the sources of the Android NDK toolchain
#  from the git server at android.git.kernel.org and package them in a nice tarball
#  that can later be used with the 'built-toolchain.sh' script.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`

# the default branch to use
BRANCH=master
register_option "--branch=<name>" BRANCH "Specify release branch"

# the default release name (use today's date)
if [ "$TOOLCHAIN_GIT_DATE" -a "$TOOLCHAIN_GIT_DATE" != "now" ] ; then
    RELEASE=`echo $TOOLCHAIN_GIT_DATE | sed -e 's!-!!g'`
else
    RELEASE=`date +%Y%m%d`
fi
register_var_option "--release=<name>" RELEASE "Specify release name"

GIT_DATE=$TOOLCHAIN_GIT_DATE
register_var_option "--git-date=<date>" GIT_DATE "Only sources that existed until specified <date>"

GITCMD=git
register_var_option "--git=<executable>" GITCMD "Use this version of the git tool"

OPTION_GIT_HTTP=no
register_var_option "--git-http" OPTION_GIT_HTTP "Use http to download sources from git"

OPTION_GIT_BASE=
register_var_option "--git-base=<git-uri>" OPTION_GIT_BASE "Use specific git repository base"

OPTION_GIT_REFERENCE=
register_var_option "--git-reference=<path>" OPTION_GIT_REFERENCE "Use local git reference base"

OPTION_PACKAGE=no
register_var_option "--package" OPTION_PACKAGE "Create source package in /tmp/ndk-$USER"

OPTION_NO_PATCHES=no
register_var_option "--no-patches" OPTION_NO_PATCHES "Do not patch sources"

PROGRAM_PARAMETERS="<src-dir>"
PROGRAM_DESCRIPTION=\
"Download the NDK toolchain sources from android.git.kernel.org into <src-dir>.
You will need to run this script before being able to rebuilt the NDK toolchain
binaries from scratch with build/tools/build-gcc.sh."

if [ -n "$TOOLCHAIN_GIT_DATE" ] ; then
  PROGRAM_DESCRIPTION="$PROGRAM_DESCRIPTION\


By default, this script will download sources from android.git.kernel.org that
correspond to the date of $TOOLCHAIN_GIT_DATE. If you want to use the tip of
tree, use '--git-date=now' instead.

If you don't want to use the official servers, use --git-base=<path> to
download the sources from another set of git repostories.

You can also speed-up the download if you maintain a local copy of the
toolchain repositories on your machine. Use --git-reference=<path> to
specify the base path that contains all copies, and its subdirectories will
be used as git clone shared references.
"

fi

extract_parameters "$@"

if [ -n "$OPTION_GIT_BASE" -a "$OPTION_GIT_HTTP" = "yes" ] ; then
    echo "ERROR: You can't use --git-base and --git-http at the same time."
    exit 1
fi

# Check that 'git' works
$GITCMD --version > /dev/null 2>&1
if [ $? != 0 ] ; then
    echo "The git tool doesn't seem to work. Please check $GITCMD"
    exit 1
fi
log "Git seems to work ok."

SRC_DIR="$PARAMETERS"
if [ -z "$SRC_DIR" -a "$OPTION_PACKAGE" = no ] ; then
    echo "ERROR: You need to provide a <src-dir> parameter or use the --package option!"
    exit 1
fi

if [ -n "$SRC_DIR" ] ; then
    mkdir -p $SRC_DIR
    SRC_DIR=`cd $SRC_DIR && pwd`
    log "Using target source directory: $SRC_DIR"
fi

# Create temp directory where everything will be copied first
#
PKGNAME=android-ndk-toolchain-$RELEASE
TMPDIR=/tmp/ndk-$USER/$PKGNAME
log "Creating temporary directory $TMPDIR"
rm -rf $TMPDIR && mkdir $TMPDIR
fail_panic "Could not create temporary directory: $TMPDIR"

# prefix used for all clone operations
if [ -n "$OPTION_GIT_BASE" ] ; then
    GITPREFIX="$OPTION_GIT_BASE"
else
    GITPROTO=git
    if [ "$OPTION_GIT_HTTP" = "yes" ] ; then
        GITPROTO=http
    fi
    GITPREFIX=${GITPROTO}://android.git.kernel.org/toolchain
fi
dump "Using git clone prefix: $GITPREFIX"

GITREFERENCE=
if [ -n "$OPTION_GIT_REFERENCE" ] ; then
    GITREFERENCE=$OPTION_GIT_REFERENCE
    if [ ! -d "$GITREFERENCE" -o ! -d "$GITREFERENCE/build" ]; then
        echo "ERROR: Invalid reference repository directory path: $GITREFERENCE"
        exit 1
    fi
    dump "Using git clone reference: $GITREFERENCE"
fi

toolchain_clone ()
{
    local GITFLAGS
    GITFLAGS=
    if [ "$GITREFERENCE" ]; then
        GITFLAGS="$GITFLAGS --shared --reference $GITREFERENCE/$1"
    fi
    dump "downloading sources for toolchain/$1"
    if [ -d "$GITPREFIX/$1" ]; then
        log "cloning $GITPREFIX/$1"
        run git clone $GITFLAGS $GITPREFIX/$1 $1
    else
        log "cloning $GITPREFIX/$1.git"
        run git clone $GITFLAGS $GITPREFIX/$1.git $1
    fi
    fail_panic "Could not clone $GITPREFIX/$1.git ?"
    log "checking out $BRANCH branch of $1.git"
    cd $1
    if [ "$BRANCH" != "master" ] ; then
        run git checkout -b $BRANCH origin/$BRANCH
        fail_panic "Could not checkout $1 ?"
    fi
    # If --git-date is used, or we have a default
    if [ -n "$GIT_DATE" ] ; then
        REVISION=`git rev-list -n 1 --until="$GIT_DATE" HEAD`
        dump "Using sources for date '$GIT_DATE': toolchain/$1 revision $REVISION"
        run git checkout $REVISION
        fail_panic "Could not checkout $1 ?"
    fi
    (printf "%-32s " "toolchain/$1.git"; git log -1 --format=oneline) >> $SOURCES_LIST
    # get rid of .git directory, we won't need it.
    cd ..
    log "getting rid of .git directory for $1."
    run rm -rf $1/.git
}

cd $TMPDIR

SOURCES_LIST=$(pwd)/SOURCES
rm -f $SOURCES_LIST && touch $SOURCES_LIST

toolchain_clone binutils
toolchain_clone build
toolchain_clone gcc
toolchain_clone gdb
toolchain_clone gmp
toolchain_clone gold  # not sure about this one !
toolchain_clone mpfr

# Patch the toolchain sources
if [ "$OPTION_NO_PATCHES" != "yes" ]; then
    PATCHES_DIR="$PROGDIR/toolchain-patches"
    if [ -d "$PATCHES_DIR" ] ; then
        dump "Patching toolchain sources"
        run $PROGDIR/patch-sources.sh $FLAGS $TMPDIR $PATCHES_DIR
        if [ $? != 0 ] ; then
            dump "ERROR: Could not patch sources."
            exit 1
        fi
    fi
fi


# We only keep one version of gcc and binutils

# we clearly don't need this
log "getting rid of obsolete sources: gcc-4.2.1 gcc-4.3.1 gcc-4.4.0 gdb-6.8 binutils-2.17"
rm -rf $TMPDIR/gcc/gcc-4.2.1
rm -rf $TMPDIR/gcc/gcc-4.3.1
rm -rf $TMPDIR/gcc/gcc-4.4.0
rm -rf $TMPDIR/gcc/gdb-6.8
rm -rf $TMPDIR/binutils/binutils-2.17

# remove all info files from the toolchain sources
# they create countless little problems during the build
# if you don't have exactly the configuration expected by
# the scripts.
#
find $TMPDIR -type f -a -name "*.info" ! -name sysroff.info -print0 | xargs -0 rm -f

if [ $OPTION_PACKAGE = "yes" ] ; then
    # create the package
    PACKAGE=/tmp/ndk-$USER/$PKGNAME.tar.bz2
    dump "Creating package archive $PACKAGE"
    pack_archive "$PACKAGE" "$TMPDIR" "."
    fail_panic "Could not package toolchain source archive ?. See $TMPLOG"
    dump "Toolchain sources downloaded and packaged succesfully at $PACKAGE"
else
    # copy sources to <src-dir>
    SRC_DIR=`cd $SRC_DIR && pwd`
    rm -rf $SRC_DIR && mkdir -p $SRC_DIR
    fail_panic "Could not create target source directory: $SRC_DIR"
    copy_directory "$TMPDIR" "$SRC_DIR"
    cp $SOURCES_LIST $SRC_DIR/SOURCES
    fail_panic "Could not copy downloaded sources to: $SRC_DIR"
    dump "Toolchain sources downloaded and copied to $SRC_DIR"
fi

dump "Cleaning up..."
rm -rf $TMPDIR
rm -f $TMPLOG
dump "Done."
