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
# manifest (i.e. AndroidManifest.xml). Usage is:
#
#   awk -f <this-script> AndroidManifest.xml
#
# The name itself is the value of the 'package' attribute in the
# 'manifest' element.
#

BEGIN {
    PACKAGE="";
    while (xml_event()) {
        # Simply extract the value of the 'name' attribute from
        # the top-level <manifest> element.
        if ( XML_TYPE == "BEGIN" && XML_RPATH == "MANIFEST/" ) {
            PACKAGE = XML_ATTR["package"];
            break;
        }
    }
    if (!PACKAGE)
        PACKAGE = "<none>";

    print PACKAGE;
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
        sub("[ \r\n\t/].*$", "", XML_TAG);  # extract tag name
        XML_TAG = toupper(XML_TAG);       # uppercase it
        if ( XML_TAG !~ /^[A-Z][-+_.:0-9A-Z]*$/ )  # validate it
            _xml_panic("Invalid tag name: " XML_TAG);
        if (XML_TYPE == "BEGIN") {  # update reverse path
            _xml_enter(XML_TAG);
        } else {
            _xml_exit(XML_TAG);
        }
        sub("[^ \r\n\t]*[ \r\n\t]*", "", $0); # get rid of tag and spaces
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
            sub(/^[ \t\r\n]*/,"",$0); # get rid of remaining leading spaces
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
