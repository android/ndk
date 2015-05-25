#!/bin/sh

# Check if ELF depends on GLIBC/GLIBCXX symbols known to be absent from
# libc.so in server

# ToDo: GLBICXX too broad (essencially ask app statically link libstdc++).
#       Only need to check GLIBCXX_3.4.9 .. GLIBCXX_3.4.18

VERBOSE=
FOUND=

check_sym()
{
    local OUT=$1
    local PASS=$2
    local SYMBOLS="\
 futimens@GLIBC_2.6\
 GLIBC_2.7\
 GLIBC_2.11\
 GLIBCXX\
"
    if [ "$PASS" = "1" ]; then
        for sym in $SYMBOLS; do
            echo "$OUT" | grep -q $sym
            if [ $? = 0 ]; then
                FOUND=yes
                return 0
	    fi
        done
	return 1
    else
        for sym in $SYMBOLS; do
            echo "$OUT" | grep $sym
        done
    fi
}

check_file()
{
    local FILE="$1"
    local OUT=`readelf -aW "$FILE" | grep GLIBC`
    check_sym "$OUT" 1
    if [ $? = 0 ]; then
        echo $FILE
	check_sym "$OUT" 2
    elif [ "$VERBOSE" = "yes" ]; then
        echo $FILE
    fi
}

check_dir()
{
    local DIR="$1"
    local FILES=`find "$DIR" -type f`
    for f in $FILES; do
        file $f | grep -q ELF
	if [ $? = 0 ]; then
            check_file $f
        fi
    done
}

ARG="$1"
if [ "$ARG" = "-v" ]; then
    ARG="$2"
    VERBOSE=yes
fi

if [ -f "$ARG" ]; then
    check_file $ARG
elif [ -d "$ARG" ]; then
    check_dir $ARG
else
    echo "Usage: check-glibc.sh [file|dir]"
    echo "$ARG is neither file nor dir"
    exit 1
fi

if [ "$FOUND" = "yes" ]; then
    exit 1
else
    exit 0
fi