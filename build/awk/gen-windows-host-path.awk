# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This script is used to generate a Makefile fragment that will be evaluated
# at runtime by the NDK build system during its initialization pass.
#
# The purpose of this generated fragment is to define a function, named
# 'cygwin-to-host-path' that will transform a Cygwin-specific path into the
# corresponding Windows specific one, i.e. calling
#
#   $(call cygwin-to-host-path,/cygdrive/c/Stuff/)  --> c:/Stuff
#
# A naive implementation of this function would be the following:
#
#   cygwin-to-host-path = $(shell cygpath -m $1)
#
# Unfortunately, calling 'cygpath -m' from GNU Make is horridly slow and people
# have complained that this was adding several minutes to their builds, even in
# the case where there is nothing to do.
#
# The script expects its input to be the output of the Cygwin "mount" command
# as in:
#
#  C:/cygwin/bin on /usr/bin type ntfs (binary,auto)
#  C:/cygwin/lib on /usr/lib type ntfs (binary,auto)
#  C:/cygwin on / type ntfs (binary,auto)
#  C: on /cygdrive/c type ntfs (binary,posix=0,user,noumount,auto)
#  D: on /cygdrive/d type udf (binary,posix=0,user,noumount,auto)
#
# The script's output will be passed to the GNU Make 'eval' function
# and will look like:
#
#  $(patsubst /%,C:/cygwin/,
#  $(patsubst /usr/bin/%,C:/cygwin/bin/,
#  $(patsubst /usr/lib/%,C:/cygwin/lib/,
#  $(patsubst /cygdrive/C/%,C:/,
#  $(patsubst /cygdrive/D/%,D:/,
#  $(patsubst /cygdrive/c/%,C:/,
#  $(patsubst /cygdrive/d/%,D:/,$1)))))
#
BEGIN {
  # setup our count
  count = 0
}

$2 == "on" {
    # record a new (host-path,cygwin-path) pair
    count ++
    host[count] = $1
    cygwin[count] = $3
}

END {
    # Drive letters are special cases because we must match both
    # the upper and lower case versions to the same drive, i.e.
    # if "mount" lists that /cygdrive/c maps to C:, we need to
    # map both /cygdrive/c and /cygdrive/C to C: in our final rules.
    #
    count1 = count
    for (nn = 1; nn <= count1; nn++) {
        if (!match(host[nn],"^[A-Za-z]:$")) {
            # not a driver letter mapping, skip this pair
            continue
        }
        letter = substr(host[nn],1,1)
        lo     = tolower(letter)
        up     = toupper(letter)

        # If the cygwin path ends in /<lo>, then substitute it with /<up>
        # to create a new pair.
        if (match(cygwin[nn],"/"lo"$")) {
            count++
            host[count] = host[nn]
            cygwin[count] = substr(cygwin[nn],1,length(cygwin[nn])-1) up
            continue
        }

        # If the cygwin path ends in /<up>, then substitute it with /<lo>
        # to create a new pair.
        if (match(cygwin[nn],"/"up"$")) {
            count++
            host[count] = host[nn]
            cygwin[count] = substr(cygwin[nn],1,length(cygwin[nn])-1) lo
            continue
        }
    }

    # We have recorded all (host,cygwin) path pairs,
    # now try to sort them so that the ones with the longest cygwin path
    # appear first
    for (ii = 2; ii <= count; ii++) {
        for (jj = ii-1; jj > 0; jj--) {
            if (length(cygwin[jj]) > length(cygwin[jj+1])) {
                break;
            }
            if (length(cygwin[jj]) == length(cygwin[jj+1]) &&
                cygwin[jj] > cygwin[jj+1]) {
                break
            }
            tmp = cygwin[jj]
            cygwin[jj] = cygwin[jj+1]
            cygwin[jj+1] = tmp
            tmp = host[jj]
            host[jj] = host[jj+1]
            host[jj+1] = tmp
        }
    }

    # build/core/init.mk defines VERBOSE to 1 when it needs to dump the
    # list of substitutions in a human-friendly format, generally when
    # NDK_LOG is defined in the environment
    #
    # Otherwise, just generate the corresponding Make function definition
    #
    if (VERBOSE == 1) {
        for (nn = 1; nn <= count; nn++) {
            printf( "$(info %s => %s)", cygwin[nn], host[nn]);
        }
    } else {
        RESULT = "$1"
        for (nn = 1; nn <= count; nn++) {
            add_drive_rule(host[nn], cygwin[nn])
        }
        print RESULT
    }
}

function add_drive_rule (hostpath,cygpath)
{
    if (cygpath == "/") {
        # Special case for /
        RESULT = "$(patsubst /%," hostpath "/%,\n" RESULT ")"
        return
    }
    # default rule otherwise
    RESULT = "$(patsubst " cygpath "/%," hostpath "/%,\n" RESULT ")"
}
