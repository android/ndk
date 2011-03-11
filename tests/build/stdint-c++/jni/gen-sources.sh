#!/bin/sh
#
# This script is used to generate the source files for this test.
#
LIMITS=""
CONSTANTS=""
for SIZE in 8 16 32 64; do
    for PREFIX in INT INT_LEAST INT_FAST; do
        LIMITS="$LIMITS $PREFIX${SIZE}_MIN $PREFIX${SIZE}_MAX"
        CONSTANTS="$CONSTANTS $PREFIX${SIZE}_C"
    done
    for PREFIX in UINT UINT_LEAST UINT_FAST; do
        LIMITS="$LIMITS $PREFIX${SIZE}_MAX"
        CONSTANTS="$CONSTANTS $PREFIX${SIZE}_C"
    done
done

LIMITS="$LIMITS INTMAX_MIN INTMAX_MAX UINTMAX_MAX"
CONSTANTS="$CONSTANTS INTMAX_C UINTMAX_C"

for PREFIX in INTPTR PTRDIFF; do
    LIMITS="$LIMITS ${PREFIX}_MIN ${PREFIX}_MAX"
    CONSTANTS="$CONSTANTS ${PREFIX}_C"
done

LIMITS="$LIMITS UINTPTR_MAX"
CONSTANTS="$CONSTANTS UINTPTR_C"

SRC=test_cpp_no_macros.cpp

gen_cpp_no_macros ()
{
    echo "/* AUTO-GENERATED FILE - DO NOT MODIFY! */"
    echo "#include <stdint.h>"
    for MACRO in $LIMITS $CONSTANTS; do
        echo "#ifdef $MACRO"
        echo "#error $MACRO defined!"
        echo "#endif"
    done
}

gen_cpp_limit_macros ()
{
    echo "/* AUTO-GENERATED FILE - DO NOT MODIFY! */"
    echo "#define __STDC_LIMIT_MACROS 1"
    echo "#include <stdint.h>"
    for MACRO in $LIMITS; do
        echo "#ifndef $MACRO"
        echo "#error $MACRO is not defined!"
        echo "#endif"
    done
    for MACRO in $CONSTANTS; do
        echo "#ifdef $MACRO"
        echo "#error $MACRO is defined!"
        echo "#endif"
    done
}

gen_cpp_constant_macros ()
{
    echo "/* AUTO-GENERATED FILE - DO NOT MODIFY! */"
    echo "#define __STDC_CONSTANT_MACROS 1"
    echo "#include <stdint.h>"
    for MACRO in $LIMITS; do
        echo "#ifdef $MACRO"
        echo "#error $MACRO is defined!"
        echo "#endif"
    done
    for MACRO in $CONSTANTS; do
        echo "#ifndef $MACRO"
        echo "#error $MACRO is not defined!"
        echo "#endif"
    done
}

gen_cpp_all_macros ()
{
    echo "/* AUTO-GENERATED FILE - DO NOT MODIFY! */"
    echo "#define __STDC_LIMIT_MACROS 1"
    echo "#define __STDC_CONSTANT_MACROS 1"
    echo "#include <stdint.h>"
    for MACRO in $LIMITS $CONSTANTS; do
        echo "#ifndef $MACRO"
        echo "#error $MACRO defined!"
        echo "#endif"
    done
}

gen_c ()
{
    echo "/* AUTO-GENERATED FILE - DO NOT MODIFY! */"
    echo "#include <stdint.h>"
    for MACRO in $LIMITS $CONSTANTS; do
        echo "#ifndef $MACRO"
        echo "#error $MACRO defined!"
        echo "#endif"
    done
}

gen_c > test_c.c
gen_cpp_no_macros > test_no_macros.cpp
gen_cpp_limit_macros > test_limit_macros.cpp
gen_cpp_constant_macros > test_constant_macros.cpp
gen_cpp_all_macros > test_all_macros.cpp
