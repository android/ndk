
# See README for details.

LIBFILE=libfoo.so

# Compile shared library
#


# NOTE: -fPIC is required for the MIPS toolchain at the moment. Without it
#        this fails with the following error:
#
#  relocation R_MIPS_HI16 against `__gnu_local_gp' can not be used when making a shared object; recompile with -fPIC
#
# Note sure this is technically a toolchain error or not.
#
if [ "$ABI" = mips ]; then
    CXXFLAGS=$CXXFLAGS" -fPIC"
fi

run ${PREFIX}g++ $LDFLAGS $CXXFLAGS -shared -o $LIBFILE foo.cpp
if [ $? != 0 ]; then
    echo "ERROR: Can't build shared library!"
    exit 1
fi

# Check that there is no .ctors section
${PREFIX}readelf -S libfoo.so | grep -q -e .ctors
if [ $? = 0 ]; then
    echo "ERROR: Shared library should not have a .ctors section!"
    exit 1
fi

# Check that there is no .dtors section
${PREFIX}readelf -S libfoo.so | grep -q -e .dtors
if [ $? = 0 ]; then
    echo "ERROR: Shared library should not have a .dtors section!"
    exit 1
fi

# Check that there is an .init_array section
${PREFIX}readelf -S $LIBFILE | grep -q -e .init_array
if [ $? != 0 ]; then
    echo "ERROR: Shared library is missing an .init_array section!"
    exit 1
fi

# Check that there is a .fini_array section
${PREFIX}readelf -S $LIBFILE | grep -q -e .fini_array
if [ $? != 0 ]; then
    echo "ERROR: Shared library is missing an .fini_array section!"
    exit 1
fi

# Everything's good
echo "Shared library is ok."
exit 0
