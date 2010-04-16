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
# A nawk/gawk script used to extract the package name from an application's
# manifest (i.e. AndroidManifest.xml).
#
# The name itself is the value of the 'package' attribute in the
# 'manifest' element.
#

BEGIN {
    FS=" "
    in_manifest=0
    package_regex1="package=\"([[:alnum:].]+)\""
    package_regex2="package='([[:alnum:].]+)'"
    PACKAGE="<none>"
}

/<manifest/ {
    in_manifest=1
}

in_manifest == 1 && /package=/ {
    if (match($0,package_regex1)) {
        PACKAGE=substr($0,RSTART+9,RLENGTH-10)
    }
    else if (match($0,package_regex2)) {
        PACKAGE=substr($0,RSTART+9,RLENGTH-10)
    }
}

in_manifest == 1 && />/ {
    in_manifest=0
}

END {
    print PACKAGE
}
