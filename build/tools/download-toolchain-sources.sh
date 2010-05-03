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

OPTION_HELP=no
OPTION_RELEASE=
OPTION_GIT=
OPTION_BRANCH=
OPTION_PACKAGE=no

# the default release name (use today's date)
RELEASE=`date +%Y%m%d`

# the default branch to use
BRANCH=master
GITCMD=git

register_option "--branch=<name>" do_branch "Specify release branch" $BRANCH
register_option "--release=<name>" do_release "Specify release name" $RELEASE
register_option "--git=<executable>" do_git "Use this version of the git tool" $GITCMD
register_option "--package" do_package "Create source package in /tmp"

do_branch () { OPTION_BRANCH=$1; }
do_release () { OPTION_RELEASE=$1; }
do_git () { OPTION_GIT=$1; }
do_package () { OPTION_PACKAGE=yes; }

PROGRAM_PARAMETERS="<src-dir>"
PROGRAM_DESCRIPTION=\
"Download the NDK toolchain sources from android.git.kernel.org into <src-dir>.
You will need to run this script before being able to rebuilt the NDK toolchain
binaries from scratch with build/tools/build-gcc.sh."

extract_parameters $@

fix_option RELEASE "$OPTION_RELEASE" "release name"

# Check that 'git' works
fix_option GITCMD  "$OPTION_GIT" "git too command"
$GITCMD --version > /dev/null 2>&1
if [ $? != 0 ] ; then
    echo "The git tool doesn't seem to work. Please check $GITCMD"
    exit 1
fi
log "Git seems to work ok."

fix_option BRANCH "$OPTION_BRANCH" "branch name"

SRC_DIR=$PARAMETERS
if [ -z "$SRC_DIR" -a $OPTION_PACKAGE = no ] ; then
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
TMPDIR=/tmp/$PKGNAME
log "Creating temporary directory $TMPDIR"
rm -rf $TMPDIR && mkdir $TMPDIR
if [ $? != 0 ] ; then
    echo "Could not create temporary directory: $TMPDIR"
fi

# prefix used for all clone operations
GITPREFIX=git://android.git.kernel.org/toolchain

toolchain_clone ()
{
    dump "downloading sources for toolchain/$1"
    log "cloning $GITPREFIX/$1.git"
    run git clone $GITPREFIX/$1.git $1
    if [ $? != 0 ] ; then
        dump "Could not clone $GITPREFIX/$1.git ?"
        exit 1
    fi
    log "checking out $BRANCH branch of $1.git"
    cd $1
    if [ "$BRANCH" != "master" ] ; then
        run git checkout -b $BRANCH origin/$BRANCH
        if [ $? != 0 ] ; then
            dump "Could not checkout $1 ?"
            exit 1
        fi
    fi
    # get rid of .git directory, we won't need it.
    cd ..
    log "getting rid of .git directory for $1."
    run rm -rf $1/.git
}

cd $TMPDIR
toolchain_clone binutils
toolchain_clone build
toolchain_clone gcc
toolchain_clone gdb
toolchain_clone gmp
#toolchain_clone gold  # not sure about this one !
toolchain_clone mpfr

# We only keep one version of gcc and binutils

# we clearly don't need this
log "getting rid of obsolete sources: gcc-4.3.1 gdb-6.8"
rm -rf $TMPDIR/gcc/gcc-4.3.1
rm -rf $TMPDIR/gcc/gdb-6.8

# remove all info files from the toolchain sources
# they create countless little problems during the build
# if you don't have exactly the configuration expected by
# the scripts.
#
find $TMPDIR -type f -a -name "*.info" -print0 | xargs -0 rm -f

if [ $OPTION_PACKAGE = "yes" ] ; then
    # create the package
    PACKAGE=/tmp/$PKGNAME.tar.bz2
    dump "Creating package archive $PACKAGE"
    cd `dirname $TMPDIR`
    TARFLAGS="cjf"
    if [ $VERBOSE = yes ] ; then
        TARFLAGS="${TARFLAGS}v"
    fi
    run tar $TARFLAGS $PACKAGE -C /tmp/$PKGNAME .
    if [ $? != 0 ] ; then
        dump "Could not package toolchain source archive ?. See $TMPLOG"
        exit 1
    fi
    dump "Toolchain sources downloaded and packaged succesfully at $PACKAGE"
else
    # copy sources to <src-dir>
    SRC_DIR=`cd $SRC_DIR && pwd`
    rm -rf $SRC_DIR && mkdir -p $SRC_DIR
    if [ $? != 0 ] ; then
        dump "ERROR: Could not create target source directory: $SRC_DIR"
        exit 1
    fi
    run cd $TMPDIR && run cp -rp * $SRC_DIR
    if [ $? != 0 ] ; then
        dump "ERROR: Could not copy downloaded sources to: $SRC_DIR"
        exit 1
    fi
    dump "Toolchain sources downloaded and copied to $SRC_DIR"
fi

dump "Cleaning up..."
rm -rf $TMPDIR
rm -f $TMPLOG
dump "Done."
