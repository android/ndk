#!/bin/sh

# Find all symlink in toolchains/windows and windows-x86_64 directories
WIN_DIRS=`find toolchains \( -name "windows" -o -name "windows-x86_64" \)`

for WIN_DIR in $WIN_DIRS; do
    while [ 1 ]
    do
        # Find all symlinks in this directory.
        SYMLINKS=`find $WIN_DIR -type l`
        if [ -z "$SYMLINKS" ]; then
            break;
        fi
        # Iterate symlinks
        for SYMLINK in $SYMLINKS; do
            if [ -L "$SYMLINK" ]; then
                DIR=`dirname "$SYMLINK"`
                FILE=`basename "$SYMLINK"`
                # Note that if `readlink $FILE` is also a link, we want to deal
                # with it in the next iteration.  There is potential infinite-loop
                # situation for cicular link doesn't exist in our case, though.
                (cd "$DIR" && \
                 LINK=`readlink "$FILE"` && \
                 test ! -L "$LINK" && \
                 rm -f "$FILE" && \
                 cp -a "$LINK" "$FILE")
            fi
        done
   done
done

# The following should print nothing if we did good job
find toolchains/ -type l | grep windows
