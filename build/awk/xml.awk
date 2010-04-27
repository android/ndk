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

# Tiny XML parser implementation in awk.
#
# This file is not meant to be used directly, instead copy the
# functions it defines here into your own script then specialize
# it appropriately.
#

# See further below for usage instructions and implementation details.
#

# ---------------------------- cut here ---------------------------

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

# ---------------------------- cut here ---------------------------

# USAGE:
#
# The functions provided here are used to extract the tags and attributes of a
# given XML file. They do not support extraction of data, CDATA, comments,
# processing instructions and declarations at all.
#
# You should use this from the BEGIN {} action of your awk script (it will
# not work from an END {} action).
#
# Call xml_event() in a while loop. This functions returns 1 for each XML
# 'event' encountered, or 0 when the end of input is reached. Note that in
# case of malformed output, an error will be printed and the script will
# force an exit(1)
#
# After each succesful xml_event() call, the following variables will be set:
#
#    XML_TYPE:  type of event: "BEGIN" -> mean an opening tag, "END" a
#               closing one.
#
#    XML_TAG:   name of the tag, always in UPPERCASE!
#
#    XML_ATTR:  a map of attributes for the type. Only set for "BEGIN" types.
#               all attribute names are in lowercase.
#
#               beware: values are *not* unescaped !
#
#    XML_RPATH: the _reversed_ element path, using "/" as a separator.
#               if you are within the <manifest><application> tag, then
#               it will be set to "APPLICATION/MANIFEST/"
#               (note the trailing slash).
#

# This is a simple example that dumps the output of the parsing.
#
BEGIN {
    while ( xml_event() ) {
        printf "XML_TYPE=%s XML_TAG=%s XML_RPATH=%s", XML_TYPE, XML_TAG, XML_RPATH;
        if (XML_TYPE == "BEGIN") {
            for (attr in XML_ATTR) {
                printf " %s='%s'", attr, XML_ATTR[attr];
            }
        }
        printf "\n";
    }
}

# IMPLEMENTATION DETAILS:
#
# 1. '>' as the record separator:
#
# RS is set to '>' to use this character as the record separator, instead of
# the default '\n'. This means that something like the following:
#
#   <foo><bar attrib="value">stuff</bar></foo>
#
# will be translated into the following successive 'records':
#
#  <foo
#  <bar attrib="value"
#  stuff</bar
#  </foo
#
# Note that the '>' is never part of the records and thus will not be matched.
# If the record does not contain a single '<', the input is either
# malformed XML, or we reached the end of file with data after the last
# '>'.
#
# Newlines in the original input are kept in the records as-is.
#
# 2. Getting rid of unwanted stuff:
#
# We don't need any of the data within elements, so we get rid of them by
# simply ignoring anything before the '<' in the current record. This is
# done with code like this:
#
#     p = index($0, "<");       # get index of '<'
#     if (p == 0) -> return 0;  # malformed input or end of file
#     $0 = substr($0, p+1);     # remove anything before the '<' in record
#
# We also want to ignore certain sections like CDATA, comments, declarations,
# etc.. These begin with a certain pattern and end with another one, e.g.
# "<!--" and "-->" for comments. This is handled by the _xml_in_section()
# function that accepts two patterns as input:
#
#    sec_begin: is the pattern for the start of the record.
#    sec_end:   is the pattern for the end of the record (minus trailing '>').
#
# The function deals with the fact that these section can embed a valid '>'
# and will then span multiple records, i.e. something like:
#
#  <!-- A comment with an embedded > right here ! -->
#
# will be decomposed into two records:
#
#   "<!-- A comment with an embedded "
#   " right here ! --"
#
# The function deals with this case, and exits when such a section is not
# properly terminated in the input.
#
# _xml_in_section() returns 1 if an ignorable section was found, or 0 otherwise.
#
# 3. Extracting the tag name:
#
# </foo> is a closing tag, and <foo> an opening tag, this is handled
# by the following code:
#
#       if (substr($0, 1, 2) == "</") {
#           XML_TYPE = "END";
#           $0 = substr($0, 3);
#       } else {
#           XML_TYPE = "BEGIN";
#           $0 = substr($0, 2);
#       }
#
# which defines XML_TYPE, and removes the leading "</" or "<" from the record.
# The tag is later extracted and converted to uppercase with:
#
#       XML_TAG = $0                      # copy record
#       sub("[ \n\t/].*$", "", XML_TAG);  # remove anything after tag name
#       XML_TAG = toupper(XML_TAG);       # conver to uppercase
#       # validate tag
#       if ( XML_TAG !~ /^[A-Z][-+_.:0-9A-Z]*$/ ) -> panic
#
# Then the record is purged from the tag name and the spaces after it:
#
#       # get rid of tag and spaces after it in $0
#       sub("[^ \n\t]*[ \n\t]*", "", $0);
#
# 4. Maintaining XML_RPATH:
#
# The _xml_enter() and _xml_exit() functions are called to maintain the
# XML_RPATH variable when entering and exiting specific tags. _xml_exit()
# will also validate the input, checking proper tag enclosure (or exit(1)
# in case of error).
#
#       if (XML_TYPE == "BEGIN") {
#           _xml_enter(XML_TAG);
#       } else {
#           _xml_exit(XML_TAG);
#       }
#
# 5. Extracting attributes:
#
# A loop is implemented to parse attributes, the idea is to get the attribute
# name, which is always followed by a '=' character:
#
#           _xml_attrib = $0;              # copy record.
#           sub(/=.*$/,"",_xml_attrib);    # get rid of '=' and anything after.
#           sub(/^[^=]*/,"",$0);           # remove attribute name from $0
#           _xml_attrib = tolower(_xml_attrib);
#           if ( _xml_attrib !~ /^[a-z][-+_0-9a-z:]*$/ )
#               _xml_panic("Invalid attribute name: " _xml_attrib);
#
# Now get the value, which is enclosed by either (") or (')
#
#          if (substr($0,1,2) == "=\"") {        # if $0 begins with ="
#               _xml_value = substr($0,3);       # extract value
#               sub(/".*$/,"",_xml_value);  
#               sub(/^="[^"]*"/,"",$0);          # remove it from $0
#           } else if (substr($0,1,2) == "='") { # if $0 begins with ='
#               _xml_value = substr($0,3);       # extract value
#               sub(/'.*$/,"",_xml_value);
#               sub(/^='[^']*'/,"",$0);          # remove it from $0
#           } else {
#               -> panic (malformed input)
#           }
#
# After that, we simply store the value into the XML_ATTR associative
# array, and cleanup $0 from leading spaces:
#
#           XML_ATTR[_xml_attrib] = _xml_value;
#           sub(/^[ \t\n]*/,"",$0);
#
#
# 6. Handling direct tag closure:
#
# When a tag is closed directly (as in <foo/>), A single '/' will be
# parsed in the attribute parsing loop. We need to record this for the
# next call to xml_event(), since the current one should return a"BEGIN"
# for the "FOO" tag instead.
#
# We do this by setting the special _xml_closing variable, as in:
#
#          if ($0 == "/") {
#               # record a delayed tag closure for the next call
#               _xml_closing = XML_TAG;
#               break
#           }
#
# This variable is checked at the start of xml_event() like this:
#
#       # delayed tag closure - see below
#       if (_xml_closing) {
#           XML_TAG = _xml_closing;
#           XML_TYPE = "END";
#           _xml_closing = "";
#           _xml_exit(XML_TAG);
#           return 1;
#       }
#
# Note the call to _xml_exit() to update XML_RPATH here.
#
