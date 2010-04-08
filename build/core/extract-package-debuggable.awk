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
# A nawk/gawk script used to extract the debuggable flag from an application's
# manifest (i.e. AndroidManifest.xml).
#
# The name itself is the value of the 'android:debuggable' attribute in the
# 'application' element.
#

BEGIN {
    FS=" "
    in_tag=0
    regex1="android:debuggable=\"true\""
    regex2="android:debuggable='true'"
    DEBUGGABLE="false"
}

/<application/ {
    in_tag=1
}

in_tag == 1 && /android:debuggable=/ {
    if (match($0,regex1)) {
        DEBUGGABLE=substr($0,RSTART+20,RLENGTH-21)
    }
    else if (match($0,regex2)) {
        DEBUGGABLE=substr($0,RSTART+20,RLENGTH-21)
    }
}

in_tag == 1 && />/ {
    in_tag=0
}

END {
    print DEBUGGABLE
}
