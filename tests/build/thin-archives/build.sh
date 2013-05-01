#!/bin/sh

$NDK/ndk-build "$@"
if [ $? != 0 ]; then
  echo "ERROR: Could not build test program!"
  exit 1
fi

# Return the type of a given file as returned by /usr/bin/file
# $1: file path
get_file_type () {
    /usr/bin/file -b "$1" 2>/dev/null
}

# Returns success iff a given file is a thin archive.
# $1: file type as returned by get_file_type()
is_file_type_thin_archive () {
  # The output of /usr/bin/file will depend on the OS:
  # regular Linux -> 'current ar archive'
  # regular Darwin -> 'current ar archive random library'
  # thin Linux -> 'data'
  # thin Darwin -> 'data'
  case "$1" in
    *"ar archive"*)
      return 1
      ;;
    "data")
      return 0
      ;;
    *)
      echo "ERROR: Unknown '$FILE_TYPE' file type" >&2
      return 2
      ;;
  esac
}

# Check that libfoo.a is a thin archive
LIBFOO_LIST=$(find obj/local -name "libfoo.a")
EXIT_CODE=0
for LIB in $LIBFOO_LIST; do
  LIB_TYPE=$(get_file_type "$LIB")
  if is_file_type_thin_archive "$LIB_TYPE"; then
    echo "OK: $LIB is a thin archive ('$LIB_TYPE')."
  else
    echo "ERROR: $LIB is not a thin archive: '$LIB_TYPE'"
    EXIT_CODE=1
  fi
done

# Check that libbar.a is not a thin archive
LIBBAR_LIST=$(find obj/local -name "libbar.a")
for LIB in $LIBBAR_LIST; do
  LIB_TYPE=$(get_file_type "$LIB")
  if is_file_type_thin_archive "$LIB_TYPE"; then
    echo "ERROR: $LIB is not a regular archive: '$LIB_TYPE'"
    EXIT_CODE=1
  else
    echo "OK: $LIB is a regular archive: '$LIB_TYPE'"
  fi
done

exit $EXIT_CODE
