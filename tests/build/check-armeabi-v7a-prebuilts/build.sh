#!/bin/sh

# The purpose of this dummy build test is to ensure that all the
# armeabi-v7a prebuilt binaries distributed with the NDK were
# properly built targetting VFPv3-D16, as per the ABI spec.
#
# For a related bug, see http://code.google.com/p/android/issues/detail?id=26199
#

#
# $1: ELF binary
# $2: Tag name (e.g. Tag_CPU_name)
#
extract_arch_tag ()
{
    echo $(readelf -A "$1" | awk '$1 == "'$2':" { print $2; }' | sort -u | tr '\n' ' ')
}

# Returns success only if a file is a static object or library.
# We simply check the suffix, which must be either .a or .o
# $1: file name
is_static_file ()
{
    case $1 in
        *.o|*.a)
            return 0
            ;;
    esac
    return 1
}

# $1: path to an ARMv7-A ELF binary (static lib, shared lib or executable)
#
check_armv7_elf_binary ()
{
    # We use a small awk script to parse the output of 'readelf -A'
    # Which typically looks like:
    #
    #
    # Attribute Section: aeabi
    #   File Attributes
    #   Tag_CPU_name: "7-A"
    #   Tag_CPU_arch: v7
    #   Tag_CPU_arch_profile: Application
    #   Tag_ARM_ISA_use: Yes
    #   Tag_THUMB_ISA_use: Thumb-2
    #   Tag_VFP_arch: VFPv3-D16
    #   Tag_ABI_PCS_wchar_t: 4
    #   Tag_ABI_FP_denormal: Needed
    #   Tag_ABI_FP_exceptions: Needed
    #   Tag_ABI_FP_number_model: IEEE 754
    #   Tag_ABI_align8_needed: Yes
    #   Tag_ABI_align8_preserved: Yes, except leaf SP
    #   Tag_ABI_enum_size: int
    #   Tag_ABI_HardFP_use: SP and DP
    #   Tag_ABI_optimization_goals: Aggressive Speed
    #   Tag_unknown_44: 1 (0x1)
    #

    echo "Checking: $(basename $1)"
    if [ ! -f "$1" ]; then
        1>&2 echo "PANIC: Missing binary: $1"
        exit 1
    fi

    # We want to check the values of Tag_CPU_name
    CPU_NAMES=$(extract_arch_tag "$1" Tag_CPU_name)
    VFP_ARCHS=$(extract_arch_tag "$1" Tag_VFP_arch)

    # IMPORTANT NOTE: Even when using -march=armv7-a, the compiler may not
    # necessarily use ARMv7-A specific instruction and will tag an object file
    # with the following attributes:
    #
    # Attribute Section: aeabi
    #   File Attributes
    #   Tag_CPU_name: "5TE"
    #   Tag_CPU_arch: v5TE
    #   Tag_ARM_ISA_use: Yes
    #   Tag_THUMB_ISA_use: Thumb-1
    #   Tag_ABI_PCS_wchar_t: 4
    #   Tag_ABI_FP_denormal: Needed
    #   Tag_ABI_FP_exceptions: Needed
    #   Tag_ABI_FP_number_model: IEEE 754
    #   Tag_ABI_align8_needed: Yes
    #   Tag_ABI_align8_preserved: Yes, except leaf SP
    #   Tag_ABI_enum_size: int
    #   Tag_ABI_optimization_goals: Aggressive Speed
    #   Tag_unknown_44: 1 (0x1)
    #
    # This means that in static libraries, you can have both
    # '5TE' and '7-A' CPU name tags at the same time, or only
    # '5TE' or only '7-A', deal with all these cases properly.

    echo "  found tags: CPU names:'$CPU_NAMES' VFP:'$VFP_ARCHS'"

    if [ "$CPU_NAMES" != "\"7-A\"" ]; then
        # This is only acceptable for static libraries or object files
        # Which end with .a and .o respectively.
        if ! is_static_file "$1"; then
            1>&2 echo "PANIC: File is not an ARMv7-A binary: $1"
            exit 1
        fi

        # We're a static library. It's ok to have only ARMv5TE code
        if [ "$CPU_NAMES" != "\"5TE\" \"7-A\"" ]; then
            return
        fi
    fi

    if [ "$VFP_ARCHS" != "VFPv3-D16" ]; then
        1>&2 echo "PANIC: File is not an armeabi-v7a binary: $1"
        1>&2 echo "Found VFP arch tag(s): $VFP_ARCHS"
        exit 1
    fi
}

GABIXX_LIBS=$NDK/sources/cxx-stl/gabi++/libs/armeabi-v7a

check_armv7_elf_binary $GABIXX_LIBS/libgabi++_shared.so
check_armv7_elf_binary $GABIXX_LIBS/libgabi++_static.a

STLPORT_LIBS=$NDK/sources/cxx-stl/stlport/libs/armeabi-v7a

check_armv7_elf_binary $STLPORT_LIBS/libstlport_shared.so
check_armv7_elf_binary $STLPORT_LIBS/libstlport_static.a

GNUSTL_LIBS=$NDK/sources/cxx-stl/gnu-libstdc++/libs/armeabi-v7a

check_armv7_elf_binary $GNUSTL_LIBS/libsupc++.a
check_armv7_elf_binary $GNUSTL_LIBS/libgnustl_shared.so
check_armv7_elf_binary $GNUSTL_LIBS/libgnustl_static.a

echo "Done!"
