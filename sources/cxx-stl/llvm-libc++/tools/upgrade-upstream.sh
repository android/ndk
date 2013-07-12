#!/bin/sh
#
# A small script used to update the content of libcxx/ to a newer
# version of libc++.
#

PROGDIR=$(dirname "$0")
PROGNAME=$(basename "$0")

# Assume this script is under tools/
NDK_LIBCXX_DIR=$(cd "$PROGDIR"/.. && pwd)

# Sanitize environment.
set -e
export LANG=C
export LC_ALL=C

# Helper functions
VERBOSE=1

run () {
  if [ "$VERBOSE" -gt 1 ]; then
    echo "COMMAND: $@"
  fi
  case $VERBOSE in
    0|1)
        "$@" >/dev/null 2>&1
        ;;
    2)
        "$@" >/dev/null
        ;;
    *)
        "$@"
        ;;
  esac
}

log () {
  if [ "$VERBOSE" -gt 1 ]; then
    printf "%s\n" "$@"
  fi
}

get_config_field () {
  cat "$NDK_LIBCXX_DIR"/upstream.config | awk '$1 == "'$1':" { print $2; }'
}

# Process command line.

DO_HELP=
NO_CLEANUP=

for opt; do
  case $opt in
    --verbose)
      VERBOSE=$(( $VERBOSE + 1 ))
      ;;
    --help)
      DO_HELP=true
      ;;
    --no-cleanup)
      NO_CLEANUP=true
      ;;
    -*)
      echo "ERROR: Unknown option '$opt'. See --help."
      exit 1
      ;;
  esac
done

if [ "$DO_HELP" ]; then
    echo "Usage: $PROGNAME [options]"
    echo ""
    echo "This script is used to update the LLVM libc++ sources to a"
    echo "more recent version."
    echo ""
    echo "Valid options:"
    echo "   --help        Print this message."
    echo "   --verbose     Increase verbosity."
    echo "   --no-cleanup  Don't remove build directory on exit."
    echo ""
    exit 0
fi

# Create build directory.
BUILD_DIR=/tmp/ndk-$USER/llvm-libc++/build
mkdir -p "$BUILD_DIR" && rm -rf "$BUILD_DIR"/*

# Ensure it is cleared when this script exits.
run_on_exit () {
  if [ -z "$NO_CLEANUP" ]; then
    # Remove temporary build directory.
    rm -rf "$BUILD_DIR"
  fi
}
trap "run_on_exit \$?" EXIT QUIT HUP TERM INT

# Get upstream SVN and revision number.
SVN_URL=$(get_config_field svn)
if [ -z "$SVN_URL" ]; then
  echo "ERROR: Can't find SVN upstream in upstream.config!"
  exit 1
fi

REVISION=$(get_config_field revision)
if [ -z "$REVISION" ]; then
  echo "ERROR: Can't find upstream revision in upstream.config!"
  exit 1
fi

run cd $BUILD_DIR &&
echo "Checking out $SVN_URL@$REVISION"
run svn co $SVN_URL@$REVISION libcxx > /dev/null
run cd libcxx

echo "Creating git repository and 'master' branch."
run git init
echo \
".gitignore
.svn/
" > .gitignore

run git add .
run git add -f .gitignore
run git commit -m "upstream @$REVISION"

echo "Create 'ndk' branch and apply patches.android/*"
run git branch ndk master
run git checkout ndk
if [ -d "$NDK_LIBCXX_DIR/patches.android" ]; then
  (
    set +e;
    run git am "$NDK_LIBCXX_DIR"/patches.android/*
    if [ "$?" != 0 ]; then
      echo "A problem occured while applying the patches!!"
      exit 1
    fi
  )
fi

echo "Updating to newer upstream revision"
run git checkout master
run git tag revision-$REVISION HEAD

run svn update
NEW_REVISION=$(svn info | awk '$1 == "Revision:" { print $2; }')
echo "Found new revision: $NEW_REVISION (was $REVISION)"

ADDED_FILES=$(git ls-files -o --exclude-standard)
MODIFIED_FILES=$(git ls-files -m)
REMOVED_FILES=$(git ls-files -d)
log "ADDED_FILES='$ADDED_FILES'"
log "MODIFIED_FILES='$MODIFIED_FILES'"
log "REMOVED_FILES='$REMOVED_FILES'"
CHANGED=
if [ -n "$ADDED_FILES" ]; then
  run git add $ADDED_FILES
  CHANGED=true
fi
if [ -n "$MODIFIED_FILES" ]; then
  run git add $MODIFIED_FILES
  CHANGED=true
fi
if [ -n "$REMOVED_FILES" ]; then
  run git rm -f $REMOVED_FILES
  CHANGED=true
fi

if [ -z "$CHANGED" ]; then
  echo "No changes detected. Exiting."
  exit 0
fi

ADDED_COUNT=$(echo "$ADDED_FILES" | wc -l)
MODIFIED_COUNT=$(echo "$MODIFIED_FILES" | wc -l)
REMOVED_COUNT=$(echo "$REMOVED_FILES" | wc -l)
echo "Commiting changes ($ADDED_COUNT new, $MODIFIED_COUNT changed, $REMOVED_COUNT deleted)"
run git commit -m "upstream @$NEW_REVISION"
run git tag revision-$NEW_REVISION

echo "Updating NDK branch."
run git checkout ndk
run git tag android-0 HEAD
run git rebase revision-$NEW_REVISION

echo "Re-creating new Android patches."
run git format-patch -k -o "$BUILD_DIR"/patches.android revision-$NEW_REVISION
run git format-patch -k -o "$BUILD_DIR"/patches.libcxx android-0

echo "Updating local sources"
run cd "$NDK_LIBCXX_DIR"/libcxx
for PATCH in "$BUILD_DIR"/patches.libcxx/*.patch; do
  (
    set +e
    run patch -p1 < "$PATCH"
    if [ $? != 0 ]; then
      echo "ERROR: Can't apply $PATCH properly!"
      exit 1
    fi
  )
done

echo "Updating local patches"
run cd "$NDK_LIBCXX_DIR"
run git rm -f patches.android/*
run cp "$BUILD_DIR"/patches.android/* patches.android/
run git add patches.android/*

echo "Updating upstream.config"
sed -i -e "s|revision: $REVISION|revision: $NEW_REVISION|" "$NDK_LIBCXX_DIR"/upstream.config
git add "$NDK_LIBCXX_DIR"/upstream.config

echo "Done updating to $NEW_REVISION."
