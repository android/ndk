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
#  from the git server at android.googlesource.com and package them in a nice tarball
#  that can later be used with the 'built-toolchain.sh' script.
#

# include common function and variable definitions
. `dirname $0`/prebuilt-common.sh
PROGDIR=`dirname $0`
PROGDIR=`cd $PROGDIR && pwd`

# the default branch to use
BRANCH=master
register_var_option "--branch=<name>" BRANCH "Specify release branch"

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

OPTION_GIT_BASE=https://android.googlesource.com/toolchain
register_var_option "--git-base=<git-uri>" OPTION_GIT_BASE "Use specific git repository base"

OPTION_GIT_REFERENCE=
register_var_option "--git-reference=<path>" OPTION_GIT_REFERENCE "Use local git reference"

OPTION_PACKAGE=no
register_var_option "--package" OPTION_PACKAGE "Create source package in /tmp/ndk-$USER"

OPTION_NO_PATCHES=no
register_var_option "--no-patches" OPTION_NO_PATCHES "Do not patch sources"

LLVM_VERSION_LIST=$DEFAULT_LLVM_VERSION_LIST
register_var_option "--llvm-version-list=<vers>" LLVM_VERSION_LIST "List of LLVM release versions"

LLVM_URL=$DEFAULT_LLVM_URL
register_var_option "--llvm-url=<url>" LLVM_URL "URL to download LLVM tar archive"

PROGRAM_PARAMETERS="<src-dir>"
PROGRAM_DESCRIPTION=\
"Download the NDK toolchain sources from android.googlesource.com into <src-dir>.
You will need to run this script before being able to rebuilt the NDK toolchain
binaries from scratch with build/tools/build-gcc.sh."

if [ -n "$TOOLCHAIN_GIT_DATE" ] ; then
  PROGRAM_DESCRIPTION="$PROGRAM_DESCRIPTION\


By default, this script will download sources from android.googlesource.com that
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

# Normalize the parameters
LLVM_VERSION_LIST=$(commas_to_spaces $LLVM_VERSION_LIST)

# Create temp directory where everything will be copied first
#
PKGNAME=android-ndk-toolchain-$RELEASE
TMPDIR=/tmp/ndk-$USER/$PKGNAME
log "Creating temporary directory $TMPDIR"
rm -rf $TMPDIR && mkdir $TMPDIR
fail_panic "Could not create temporary directory: $TMPDIR"

# prefix used for all clone operations
GITPREFIX="$OPTION_GIT_BASE"
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

# Clone a given toolchain git repository
# $1: repository name, relative to $GITPREFIX (e.g. 'gcc')
#
toolchain_clone ()
{
    local GITFLAGS
    GITFLAGS="--no-checkout"
    if [ "$GITREFERENCE" ]; then
        GITFLAGS=$GITFLAGS" --shared --reference $GITREFERENCE/$1"
    fi
    dump "Cloning git repository for toolchain/$1"
    if [ -d "$GITPREFIX/$1" ]; then
        run ln -s "$GITPREFIX/$1" $CLONE_DIR/$1.git
    else
        log "cloning $GITPREFIX/$1.git"
        (cd $CLONE_DIR && run $GITCMD clone $GITFLAGS $GITPREFIX/$1.git)
    fi
    fail_panic "Could not clone $GITPREFIX/$1.git ?"
}

# Checkout sources from a git clone created with toolchain_clone
# $1: sub-directory
# $2: branch (e.g. 'master')
# $3: repository/clone name (e.g. 'gcc')
# $4+: sub-path to extract, relative to clone top-level (e.g. 'gcc-4.6')
#
toolchain_checkout ()
{
    local SUBDIR=$1
    local BRANCH=$2
    local NAME=$3
    shift ; shift ; shift
    local GITOPTS="--git-dir=$CLONE_DIR/$NAME/.git"
    log "Checking out $BRANCH branch of $NAME.git: $@"
    local REVISION=origin/$BRANCH
    if [ -n "$GIT_DATE" ] ; then
        REVISION=`$GITCMD $GITOPTS rev-list -n 1 --until="$GIT_DATE" $REVISION`
    fi
    (mkdir -p $TMPDIR/$SUBDIR/$NAME && cd $TMPDIR/$SUBDIR/$NAME && run $GITCMD $GITOPTS checkout $REVISION "$@")
    fail_panic "Could not checkout $NAME / $@ ?"
    if [ "$BRANCH" = "master" ]; then
        BRANCH=
    else
        BRANCH="($BRANCH)"
    fi
    (printf "%-38s " "toolchain/$NAME.git $BRANCH"; $GITCMD $GITOPTS log -1 --format=oneline $REVISION) >> $SOURCES_LIST
}

cd $TMPDIR

CLONE_DIR=$TMPDIR/git
mkdir -p $CLONE_DIR

SOURCES_LIST=$(pwd)/SOURCES
rm -f $SOURCES_LIST && touch $SOURCES_LIST

toolchain_clone build
toolchain_clone gmp
toolchain_clone mpfr
toolchain_clone mpc
toolchain_clone cloog
toolchain_clone isl
toolchain_clone ppl
toolchain_clone expat
toolchain_clone binutils
toolchain_clone gcc
toolchain_clone gdb
toolchain_clone python
toolchain_clone perl
toolchain_clone clang
toolchain_clone llvm
toolchain_clone compiler-rt
toolchain_clone mclinker

toolchain_checkout "" $BRANCH build .
toolchain_checkout "" $BRANCH gmp .
toolchain_checkout "" $BRANCH mpfr .
toolchain_checkout "" $BRANCH mpc .
toolchain_checkout "" $BRANCH cloog .
toolchain_checkout "" $BRANCH isl .
toolchain_checkout "" $BRANCH ppl .
toolchain_checkout "" $BRANCH expat .
toolchain_checkout "" $BRANCH binutils binutils-2.19 binutils-2.21 binutils-2.22 binutils-2.23
toolchain_checkout "" $BRANCH gcc gcc-4.6 gcc-4.7 gcc-4.8
toolchain_checkout "" $BRANCH gdb gdb-7.3.x gdb-7.6
toolchain_checkout "" $BRANCH python Python-2.7.5
toolchain_checkout "" $BRANCH perl perl-5.16.2
toolchain_checkout "" $BRANCH mclinker .

for LLVM_VERSION in $LLVM_VERSION_LIST; do
    # Check-out and Adjust directory structure a bit
    # 1. Create symbolic link for clang which is always built
    # 2. Create symbolic link for compiler-rt too
    # 3. Move tools/polly up to be a sibling of llvm and clang.  Script build-llvm.sh
    #    will only create symbolic link when --with-polly is specified.
    LLVM_VERSION_NO_DOT=$(echo $LLVM_VERSION | sed -e 's!\.!!g')
    LLVM_BRANCH="release_$LLVM_VERSION_NO_DOT"
    toolchain_checkout "llvm-$LLVM_VERSION" $LLVM_BRANCH clang .
    toolchain_checkout "llvm-$LLVM_VERSION" $LLVM_BRANCH llvm .
    (cd "$TMPDIR/llvm-$LLVM_VERSION/llvm" && \
        ln -s ../../clang tools && \
        test -d tools/polly && mv tools/polly ..)
    if [ "$LLVM_VERSION" != "3.1" ]; then
        # compiler-rt only exists on and after 3.2
        toolchain_checkout "llvm-$LLVM_VERSION" $LLVM_BRANCH compiler-rt .
        (cd "$TMPDIR/llvm-$LLVM_VERSION/llvm" && \
            test -d ../compiler-rt && ln -s ../../compiler-rt projects)
    fi
    # In polly/utils/cloog_src, touch Makefile.in, aclocal.m4, and configure to
    # make sure they are not regenerated.
    (test -d "$TMPDIR/llvm-$LLVM_VERSION/polly" && \
     cd "$TMPDIR/llvm-$LLVM_VERSION/polly" && \
        find . -name "Makefile.in" -exec touch {} \; && \
        find . -name "aclocal.m4" -exec touch {} \; && \
        find . -name "configure" -exec touch {} \; )
done

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
    cp $SOURCES_LIST $SRC_DIR/SOURCES
    fail_panic "Could not copy $SOURCES_LIST to $SRC_DIR"
    mv "$TMPDIR"/* "$SRC_DIR"  #copy_directory "$TMPDIR" "$SRC_DIR"
    fail_panic "Could not move to target source directory: $TMPDIR -> $SRC_DIR"
    dump "Toolchain sources downloaded and copied to $SRC_DIR"
fi

dump "Cleaning up..."
rm -rf $TMPDIR
rm -f $TMPLOG
dump "Done."
