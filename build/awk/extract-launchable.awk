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
# A nawk/gawk script used to extract the list of launchable activities
# from an application's manifest (i.e. AndroidManifest.xml). Usage:
#
#   awk -f <this-script> AndroidManifest.xml
#

#
# Explanation:
#
# A given application can have several activities, and each activity
# can have several intent filters. We want to only list, in the final
# output, the activities which have a intent-filter that contains the
# following elements:
#
#   <action android:name="android.intent.action.MAIN" />
#   <category android:name="android.intent.category.LAUNCHER" />
#
# To do this, we need hooks called when entering and exiting <activity>
# and <intent-filter> elements.
#

BEGIN {
    while ( xml_event() ) {
        # concat xml event type and tag for simpler comparisons
        event = XML_TYPE "-" XML_TAG;
        # When entering a new <activity>, extract its name and set
        # the 'launchable' flag to false.
        if ( event == "BEGIN-ACTIVITY" && 
             XML_RPATH == "ACTIVITY/APPLICATION/MANIFEST/" ) {
            name = XML_ATTR["android:name"];
            launchable = 0;
        }
        # When exiting an <activity>, check that it has a name and
        # is launchable. If so, print its name to the output
        else if ( event == "END-ACTIVITY" &&
                  XML_RPATH == "APPLICATION/MANIFEST/" ) {
            if ( name && launchable ) {
                print name;
            }
        }
        # When entering an <intent-filter> inside an <activity>, clear
        # the 'action' and 'category' variables. They are updated when
        # we enter the corresponding elements within the intent-filter.
        else if ( event == "BEGIN-INTENT-FILTER" &&
                 XML_RPATH == "INTENT-FILTER/ACTIVITY/APPLICATION/MANIFEST/" ) {
            action = ""
            category = ""
        }
        # When exiting an <intent-filter>, set the 'launchable' flag to true
        # for the current activity if both 'action' and 'category' have the
        # correct name.
        else if ( event == "END-INTENT-FILTER" &&
                  XML_RPATH == "ACTIVITY/APPLICATION/MANIFEST/" ) {
            if ( action == "android.intent.action.MAIN" &&
                    category == "android.intent.category.LAUNCHER" ) {
                    launchable = 1;
            }
        }
        # When entering an <action> element inside an <intent-filter>, record
        # its name.
        else if ( event == "BEGIN-ACTION" &&
                  XML_RPATH == "ACTION/INTENT-FILTER/ACTIVITY/APPLICATION/MANIFEST/" ) {
            action = XML_ATTR["android:name"];
        }
        # When entering a <category> element inside an <intent-filter>, record
        # its name.
        else if ( event == "BEGIN-CATEGORY" &&
                  XML_RPATH == "CATEGORY/INTENT-FILTER/ACTIVITY/APPLICATION/MANIFEST/" ) {
            category = XML_ATTR["android:name"];
        }
    }
}


#
# the following is copied directly from xml.awk - see this file for
# usage and implementation details.
#
function xml_event () {
    RS=">";
    XML_TAG=XML_TYPE="";
    split("", XML_ATTR);
    while ( 1 ) {
        if (_xml_closing) { # delayed direct tag closure
            XML_TAG = _xml_closing;
            XML_TYPE = "END";
            _xml_closing = "";
            _xml_exit(XML_TAG);
            return 1;
        }
        if (getline <= 0) return 0; # read new input line
        _xml_p = index($0, "<"); # get start marker
        if (_xml_p == 0) return 0; # end of file (or malformed input)
        $0 = substr($0, _xml_p) # remove anything before '<'
        # ignore CData / Comments / Processing instructions / Declarations
        if (_xml_in_section("<!\\[[Cc][Dd][Aa][Tt][Aa]\\[", "]]") ||
            _xml_in_section("<!--", "--") ||
            _xml_in_section("<\\?", "\\?") ||
            _xml_in_section("<!", "")) {
            continue;
        }
        if (substr($0, 1, 2) == "</") { # is it a closing tag ?
            XML_TYPE = "END";
            $0 = substr($0, 3);
        } else { # nope, it's an opening one
            XML_TYPE = "BEGIN";
            $0 = substr($0, 2);
        }
        XML_TAG = $0
        sub("[ \n\t/].*$", "", XML_TAG);  # extract tag name
        XML_TAG = toupper(XML_TAG);       # uppercase it
        if ( XML_TAG !~ /^[A-Z][-+_.:0-9A-Z]*$/ )  # validate it
            _xml_panic("Invalid tag name: " XML_TAG);
        if (XML_TYPE == "BEGIN") {  # update reverse path
            _xml_enter(XML_TAG);
        } else {
            _xml_exit(XML_TAG);
        }
        sub("[^ \n\t]*[ \n\t]*", "", $0); # get rid of tag and spaces
        while ($0) { # process attributes
            if ($0 == "/") {  # deal with direct closing tag, e.g. </foo>
                _xml_closing = XML_TAG; # record delayed tag closure.
                break
            }
            _xml_attrib = $0;
            sub(/=.*$/,"",_xml_attrib);  # extract attribute name
            sub(/^[^=]*/,"",$0);         # remove it from record
            _xml_attrib = tolower(_xml_attrib);
            if ( _xml_attrib !~ /^[a-z][-+_0-9a-z:]*$/ ) # validate it
                _xml_panic("Invalid attribute name: " _xml_attrib);
            if (substr($0,1,2) == "=\"") { # value is ="something"
                _xml_value = substr($0,3);
                sub(/".*$/,"",_xml_value);
                sub(/^="[^"]*"/,"",$0);
            } else if (substr($0,1,2) == "='") { # value is ='something'
                _xml_value = substr($0,3);
                sub(/'.*$/,"",_xml_value);
                sub(/^='[^']*'/,"",$0);
            } else {
                _xml_panic("Invalid attribute value syntax for " _xml_attrib ": " $0);
            }
            XML_ATTR[_xml_attrib] = _xml_value;  # store attribute name/value
            sub(/^[ \t\n]*/,"",$0); # get rid of remaining leading spaces
        }
        return 1; # now return, XML_TYPE/TAG/ATTR/RPATH are set
    }
}

function _xml_panic (msg) {
    print msg > "/dev/stderr"
    exit(1)
}

function _xml_in_section (sec_begin, sec_end) {
    if (!match( $0, "^" sec_begin )) return 0;
    while (!match($0, sec_end "$")) {
        if (getline <= 0) _xml_panic("Unexpected EOF: " ERRNO);
    }
    return 1;
}

function _xml_enter (tag) {
    XML_RPATH = tag "/" XML_RPATH;
}

function _xml_exit (tag) {
    _xml_p = index(XML_RPATH, "/");
    _xml_expected = substr(XML_RPATH, 1, _xml_p-1);
    if (_xml_expected != XML_TAG)
        _xml_panic("Unexpected close tag: " XML_TAG ", expecting " _xml_expected);
    XML_RPATH = substr(XML_RPATH, _xml_p+1);
}
