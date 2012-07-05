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


#
# WARNING: VERY IMPORTANT TECHNICAL NOTE:
#
# The function below works by inspecting the architecture-specific
# attributes in an ELF file. Please be aware that the behaviour of
# binutils-2.19 and binutils-2.21 is different when generating these
# tags.
#
# 1/ When compiling for ARMv7-A targets, one can use any of the following
#    labels for the -mfpu=<name> option:
#
#        vfp
#        vfpv3
#        vfpv3-d16
#        neon
#
# 2/ There are two VFPv3 architectures defined by ARM:
#
#        VFPv3-D16  -> Mandates only 16 double FPU registers (d0-d15)
#        VFPv3-D32  -> Mandates 32 double FPU registers (d0-d31)
#
#    In addition, NEON requires VFPv3-D32
#
#    There is also VFPv2, which is an earlier version of VFPv3. Technically
#    speaking, VFPv3 is not completely backwards compatible with VFPv2 because
#    there are a few VFPv2 instructions it doesn't support.
#
# 3/ The table below indicates, for each -mfpu label, the following:
#
#     - The value of the 'Tag_VFP_arch' attribute that will be placed in
#       the generated object files or binaries (you can list them with
#       'readelf -A <file>')
#
#     - Whether the generated code uses 16 or 32 FPU double registers
#       (this is checked by looking at the disassembly of libgnustl_shared.so,
#       more specifically functions like 'cosf' or 'sinf' inside it).
#
#  First, for binutils-2.19:
#
#     fpu value           EABI tag          FPU reg count
#    -----------------------------------------------------
#       vfp                 VFPv2            16
#       vfpv3               VFPv3-D16        32 (*)
#       vfpv3-d16           VFPv3            16 (*)
#       neon                VFPv3            32
#
#  And now for binutils-2.21
#
#     fpu value           EABI tag          FPU reg count
#    -----------------------------------------------------
#       vfp                 VFPv2            16
#       vfpv3               VFPv3            32
#       vfpv3-d16           VFPv3-D16        16
#       neon                VFPv3            32
#
#  This shows that:
#
#    - The 'VFPv3' tag seems to match VFPv3-D32 exclusively on 2.21,
#      but is a mess with 2.19
#
#    - Similarly, the 'vfpv3' value seems to match VFPv3-D32 as well,
#      with the exception that binutils-2.19 is buggy and will put an
#      invalid tag (VFPv3-D16, instead of VFPv3) in the generate ELF file.
#
#    - binutils 2.19 puts the wrong tag in the executable for vfpv3 and
#      vfpv3-d16, then should probably be inverted!
#
#  The end result is that we can't use the EABI tag to determine the number
#  of hardware FPU registers that are really used by the machine code with
#  binutils 2.19 :-(
#
#  BONUS:
#
#    - When using 'neon', binutils-2.21 will also add a new tag named
#      'Tag_Advanced_SIMD_arch' with value 'NEONv1'. Sadly, binutils-2.19
#      doesn't do any of this.
#

# Check that an ELF binary is compatible with our armeabi-v7a ABI
# (i.e. no NEON, and only 16 hardware registers being used).
#
# See technical note above to understand how this currently works.
# We're still assuming the toolchain is built with the buggy binutils-2.19.
#
# $1: path to an ARMv7-A ELF binary (static lib, shared lib or executable)
#
check_armv7_elf_binary ()
{
    # We use a small awk script to parse the output of 'readelf -A'
    # Which typically looks like:
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
    # Note that for static libraries, these sections will appear multiple
    # time in the output of 'readelf -A'.

    echo "Checking: $(basename $1)"
    if [ ! -f "$1" ]; then
        1>&2 echo "PANIC: Missing binary: $1"
        exit 1
    fi

    # We want to check the values of Tag_CPU_name
    CPU_NAMES=$(extract_arch_tag "$1" Tag_CPU_name)
    VFP_ARCHS=$(extract_arch_tag "$1" Tag_VFP_arch)
    NEON_ARCHS=$(extract_arch_tag "$1" Tag_Advanced_SIMD_arch)

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

    echo "  found tags: CPU names:'$CPU_NAMES' VFP:'$VFP_ARCHS' NEON:'$NEON_ARCHS'"

    # Clearly, any trace of NEON is a deal-breaker!
    if [ "$NEON_ARCHS" ]; then
        1>&2 echo "PANIC: Binary file should not contain NEON instructions: $1"
        exit 1
    fi

    if is_static_file "$1"; then
        # For static libraries / object files, it's ok to contain ARMv5TE binaries
        if [ "$CPU_NAMES" == "\"5TE\"" -a "$CPU_NAMES" != "\"7-A\"" -a "$CPU_NAMES" != "\"5TE\" \"7-A\"" ]; then
            # Neither ARMv7-A or ARMv5TE+ARMv7-A, something's fishy
            1>&2 echo "PANIC: File is neither ARMv5TE or ARMv7-A binary: $1"
            exit 1
        fi

        # exit here because some static libraries can have a mix of several
        # VFP tags that make them difficult to check (e.g. libgnustl_static.a
        # can have 'VFPv1 VFPv2 VFPv3' at the same time :-(
        return
    fi

    # If we reach this point, we only contain ARMv7-A machine code, so look
    # at the VFP arch tag(s)

    # Sometimes no VFP_arch tag is placed in the final binary, this happens
    # with libgabi++_shared.so for example, because the code doesn't have
    # any floating point instructions.
    #

    # XXX: FOR NOW, ASSUME BROKEN binutils-2.19, AND THUS THAT 'VFPv3' IS VALID

    if [ "$VFP_ARCHS" != "VFPv3" -a "$VFP_ARCHS" != "VFPv3-D16" -a "$VFP_ARCHS" != "" ]; then
        1>&2 echo "PANIC: File is not a VFPv3-D16 binary: $1"
        exit 1
    fi
}

GABIXX_LIBS=$NDK/sources/cxx-stl/gabi++/libs/armeabi-v7a

check_armv7_elf_binary $GABIXX_LIBS/libgabi++_shared.so
check_armv7_elf_binary $GABIXX_LIBS/libgabi++_static.a

STLPORT_LIBS=$NDK/sources/cxx-stl/stlport/libs/armeabi-v7a

check_armv7_elf_binary $STLPORT_LIBS/libstlport_shared.so
check_armv7_elf_binary $STLPORT_LIBS/libstlport_static.a

. $NDK/build/tools/dev-defaults.sh

for VERSION in $DEFAULT_GCC_VERSION_LIST; do
    GNUSTL_LIBS=$NDK/sources/cxx-stl/gnu-libstdc++/$VERSION/libs/armeabi-v7a
    check_armv7_elf_binary $GNUSTL_LIBS/libsupc++.a
    check_armv7_elf_binary $GNUSTL_LIBS/libgnustl_shared.so
    check_armv7_elf_binary $GNUSTL_LIBS/libgnustl_static.a
done

echo "Done!"
