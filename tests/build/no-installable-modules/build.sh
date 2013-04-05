# Check that if there are no installable modules (i.e. shared libraries
# or executables), ndk-build will automatically build static library
# modules.
$NDK/ndk-build "$@"
if [ "$?" != 0 ]; then
  echo "ERROR: Could not build project!"
  exit 1
fi

# Check that libfoo.a was built properly.
LIBFOO_LIBS=$(find . -name "libfoo.a" 2>/dev/null)
if [ -z "$LIBFOO_LIBS" ]; then
  echo "ERROR: Could not find libfoo.a anywhere:"
  tree .
  exit 1
fi

# Check that libcpufeatures.a was _not_ built because it was not
# a top-level module, but an imported one.
CPUFEATURES_LIBS=$(find . -name "libcpufeatures.a" 2>/dev/null)
if [ -n "$CPUFEATURES_LIBS" ]; then
  echo "ERROR: Should not find libcpufeatures.a in output directory:"
  tree .
  exit 1
fi

