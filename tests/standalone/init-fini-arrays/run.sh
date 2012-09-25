
# See README for details.

LIBFILE=libfoo.so

# Compile shared library
#

$CXX $LDFLAGS $CXXFLAGS -shared -o $LIBFILE foo.cpp
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
