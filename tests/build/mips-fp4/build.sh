cd $(dirname "$0")

$NDK/ndk-build -B "$@"

# Find instruction in file
# $1: instruction
# $2: file
find_instruction ()
{
    local INST=$1
    local FILE=$2

    grep -q -w -F -e $INST $FILE
    if [ $? != 0 ]; then
        echo "$INST expected in file $FILE"
        exit 1
    fi
}

find_instruction "madd.d" mips-fp4-test1-2.s
find_instruction "msub.d" mips-fp4-test1-2.s
find_instruction "nmadd.d" mips-fp4-test3-6.s
find_instruction "nmsub.d" mips-fp4-test3-6.s
find_instruction "recip.d" mips-fp4-test3-6.s
find_instruction "rsqrt.d" mips-fp4-test3-6.s

rm -f *.s *.i
