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

# Extract the pid of a given package name. This assumes that the
# input is the product of 'adb shell ps' with all \r\n line endings
# converted to \n, and that the PACKAGE variable has been initialized
# to the package's name. In other words, this should be used as:
#
#   adb shell ps | awk -f <this-script> -v PACKAGE=<name>
#
# The printed value will be 0 if the package is not found.
#

BEGIN {
    FS=" "

    # A default package name, used _only_ for unit-testing
    # com.google.android.apps.maps is interesting because
    # in our unit test input files, 'ps' lists several sub-processes
    # that implement services (e.g. com.google.android.apps.maps:<something>)
    # and we explicitely don't want to match them.
    #
    if (PACKAGE == "") {
        PACKAGE="com.google.android.apps.maps"
    }

    PID=0

    # The default column where we expect the PID to appear, this
    # matches the default Android toolbox 'ps', but some devices seem
    # to have a different version installed (e.g. Busybox) that place
    # it somewhere else. We will probe the output to detect this, but
    # this is a good fallback value.
    PID_COLUMN=2
}

{
    # First, remove any trailing \r from the input line. This is important
    # because the output of "adb shell <cmd>" seems to use \r\n line ending.
    gsub("\r","",$NF)

    if (NR == 1) {
        # The first line of the 'ps' output should list the columns, so we're going
        # to parse it to try to update PID_COLUMN
        for (n = 1; n <= NF; n++) {
            if ($n == "PID") {
                PID_COLUMN=n;
            }
        }
    } else {
        # Not the first line, compare the package name, which shall always
        # be the last field.
    if ($NF == PACKAGE) {
        PID=$PID_COLUMN
        }
    }
}

END {
    print PID
}
