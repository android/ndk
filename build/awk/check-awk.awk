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

# This script is used to check that a given awk executable
# implements the match() and substr() functions appropriately.
#
# These were introduced in nawk/gawk, but the original awk
# does not have them.
#
BEGIN {
    RSTART=0
    RLENGTH=0
    s1="A real world example"
    if (! match(s1,"world")) {
        print "Fail match"
    } else if (RSTART != 8) {
        print "Fail RSTART ="RSTART
    } else if (RLENGTH != 5) {
        print "Fail RLENGTH ="RLENGTH
    } else {
        s2=substr(s1,RSTART,RLENGTH)
        if (s2 != "world") {
            print "Fail substr="s2
        } else {
            print "Pass"
        }
    }
}
