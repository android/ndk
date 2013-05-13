#!/usr/bin/env python

# A tiny Python script to perform substitutions in the NDK documentation
# .text input files before processing them with Markdown.
#

import re
import argparse
import sys

class Filter:
  def __init__(self,pattern,replacement):
    self.pattern = re.compile(pattern)
    self.replacement = replacement

  def process(self, line):
    return self.pattern.sub(self.replacement, line)

all_filters = []
all_filter_tests = []

def add_filter(pattern, replacement):
  global all_filters
  filter = Filter(pattern, replacement)
  all_filters.append(filter)

def add_filter_test(input, expected):
  global all_filter_tests
  all_filter_tests.append((input, expected))

def run_all_tests():
  global all_filter_tests
  count = 0
  failed_tests = []
  for input_string, expected in all_filter_tests:
    string = input_string
    print "Testing: '%s'" % input_string,
    for f in all_filters:
      string = f.process(string)
    if string != expected:
      failed_tests.append((input_string, expected, string))
      print "  KO!"
      print "  Got     : '%s'" % string
      print "  Expected: '%s'" % expected
    else:
      print "ok."
    count += 1

  return count, failed_tests

# Auto-linkify documentation
#
#     d/NDK-BUILD
# -> [NDK-BUILD](NDK-BUILD.html)
#
add_filter(r"(^|\s+)d/([^\s.]+)", r"\1[\2](\2.html)")

add_filter_test("d/NDK-BUILD", "[NDK-BUILD](NDK-BUILD.html)")
add_filter_test("aa d/NDK-BUILD", "aa [NDK-BUILD](NDK-BUILD.html)")
add_filter_test("ad/NDK-BUILD", "ad/NDK-BUILD")
add_filter_test("d/NDK-BUILD.", "[NDK-BUILD](NDK-BUILD.html).")

# Auto-linkify documentation
#    NDK-BUILD.html
# -> [NDK-BUILD](NDK-BUILD.html)
#
add_filter(r"(^|\s+)([A-Z0-9-]+)\.html", r"\1[\2](\2.html)")
add_filter_test("NDK-BUILD.html", "[NDK-BUILD](NDK-BUILD.html)")
add_filter_test("NDK-BUILD.html.", "[NDK-BUILD](NDK-BUILD.html).")
add_filter_test("aa NDK-BUILD.html", "aa [NDK-BUILD](NDK-BUILD.html)")

add_filter(r"(^|\s+)(\$NDK/docs/|docs/)([A-Z0-9_-]+)\.html", r"\1[\3](\3.html)")
add_filter_test("$NDK/docs/ANDROID-MK.html", "[ANDROID-MK](ANDROID-MK.html)")
add_filter_test("See docs/ANDROID-MK.html.", "See [ANDROID-MK](ANDROID-MK.html).")

# Auto quote script file.
#    make-standalone-toolchain.sh
# -> `make-standalone-toolchain.sh`
add_filter(r"(^|\s+)([^\s]+\.sh)", r"\1`\2`")
add_filter_test("make-standalone-toolchain.sh", "`make-standalone-toolchain.sh`")

# Auto-linkify bug entries:
#
#    http://b.android.com/<number>
# or http://code.google.com/p/android/issues/detail?id=<number>
# -> [b/<number>](http://b.android.com/<number>)
#
add_filter(
  r"http://(code\.google\.com/p/android/issues/detail\?id=|b\.android\.com/)([0-9]+)",
  r"[b/\2](http://b.android.com/\2)")
add_filter_test(r"See http://b.android.com/12345", r"See [b/12345](http://b.android.com/12345)")
add_filter_test(r"See http://code.google.com/p/android/issues/detail?id=12345", r"See [b/12345](http://b.android.com/12345)")

# Auto-linkify bug shortcuts like b/1000
#
#    b/<number> after space or start of line
# -> [b/<number>](http://b.android.com/<number>)
add_filter(
  r"(^|\s+)(b/([0-9]+))",
  r"\1[\2](http://b.android.com/\3)")
add_filter_test(r"b/12345", r"[b/12345](http://b.android.com/12345)")
add_filter_test(r"See b/12345.", r"See [b/12345](http://b.android.com/12345).")
add_filter_test(r"[b/12345](....)", r"[b/12345](....)")

# Auto-linkify patch entries.
#    https://android-review.googlesource.com/#/c/<number>
# -> [r/<number>](https://android-review.googlesource.com/#/c/<number>)
add_filter(
  r"(^|\s+)(https://android-review\.googlesource\.com/\#/c/([0-9]+))",
  r"\1[r/\3](\2)")
add_filter_test(r"https://android-review.googlesource.com/#/c/12345", r"[r/12345](https://android-review.googlesource.com/#/c/12345)")

# Auto-linkify anything
#    http://www.example.com
# -> <http://www.example.com>
add_filter(r"(^|\s+)((ssh|http|https|ftp)://[^\s]+)", r"\1<\2>")
add_filter_test("http://example.com", "<http://example.com>")


#    r/<number> not followed by (...)
# -> [r/<number>](https://android-review.googlesource.com/#/c/<number>)
add_filter(
  r"(^|\s+)(r/([0-9]+))",
  r"\1[\2](https://android-review.googlesource.com/#/c/\3)")
add_filter_test(
  r"r/12345",
  r"[r/12345](https://android-review.googlesource.com/#/c/12345)")

# Auto format __ANDROID__, __ARM_ARCH*__, etc..
#    __XXX__
# -> `__XXX__`
add_filter(r"(__[A-Z][^\s]*)", r"`\1`")
add_filter_test(r"__ANDROID__", r"`__ANDROID__`")
add_filter_test(r"__ARM_ARCH_5*__", r"`__ARM_ARCH_5*__`")

# Auto-format compiler/linker flags:
#    -O2
# -> `-O2`
add_filter(r"(^|\s+)(\-[\w][^\s]+)", r"\1`\2`")
add_filter_test(r"-O2", r"`-O2`")
add_filter_test(r" -fPIC", r" `-fPIC`")
add_filter_test(r" -mfpu=neon xxx", r" `-mfpu=neon` xxx")
add_filter_test(r" -mfpu=vfpd3-d16", r" `-mfpu=vfpd3-d16`")

# Auto-format LOCAL_XXX, APP_XXX and NDK_XXX variables
# as well as assignments.
add_filter(r"(^|\s+)([A-Z_0-9]+=[^\s]+)", r"\1`\2`")
add_filter_test("Use NDK_DEBUG=release", "Use `NDK_DEBUG=release`")
add_filter_test("NDK_HOST_32BIT=1", "`NDK_HOST_32BIT=1`")

add_filter(r"(^|\s+)((APP_|NDK_|LOCAL_)[A-Z0-9_]*)", r"\1`\2`")
add_filter_test("LOCAL_MODULE", "`LOCAL_MODULE`")

# Auto-format __cxa_xxxxx and other prefixes.
#
add_filter(r"(^|\s+)((__cxa_|__dso_|__aeabi_|__atomic_|__sync_)[A-Za-z0-9_]+)", r"\1`\2`")
add_filter_test("__cxa_begin_cleanup", "`__cxa_begin_cleanup`")
add_filter_test("__dso_handle", "`__dso_handle`")
add_filter_test("__aeabi_idiv2", "`__aeabi_idiv2`")
add_filter_test("See __cxa_cleanup.", "See `__cxa_cleanup`.")

re_blockquote = re.compile(r"^        ")

def process(input_file, output_file):
  # Process lines, we need to take care or _not_ processing
  # block-quoted lines. For our needs, these begin with 8 spaces.
  #
  in_list = False
  margins = [ 0 ]
  margin_index = 0
  for line in input_file:
    do_process = True
    if len(line.strip()):
      if not re_blockquote.match(line):
        for f in all_filters:
          line = f.process(line)
    output_file.write(line)

def main():
    parser = argparse.ArgumentParser(description='''
    Perform .text substitution before Markdown processing.''')

    parser.add_argument( '-o', '--output',
                         help="Specify output file, stdout otherwise.",
                         dest='output',
                         default=None )

    parser.add_argument( '--run-checks',
                         help="Run internal unit tests.",
                         action='store_true',
                         dest='run_checks',
                         default=False )

    parser.add_argument( 'input_file',
                         help="Input file, stdin if not specified.",
                         nargs="?",
                         default=None )

    args = parser.parse_args()

    if args.run_checks:
      count, failed_tests = run_all_tests()
      if failed_tests:
        sys.stderr.write("ERROR: %d tests out of %d failed:\n" % (len(failed_tests), count))
        for failed in failed_tests:
          sys.stderr.write("  '%s' -> '%s' (expected '%s')\n" % (failed[0], failed[2], failed[1]))
        sys.exit(1)
      else:
        print "%d tests passed. Congratulations!" % count
        sys.exit(0)

    if args.input_file:
      try:
        in_file = open(args.input_file, "rt")
      except:
        sys.stderr.write("Error: Can't read input file: %s: %s\n" % args.input_file, repr(e))
        sys.exit(1)
    else:
      in_file = sys.stdin

    if args.output:
      try:
        out_file = open(args.output, "wt")
      except:
        sys.stderr.write("Error: Can't open output file: %s: %s\n" % args.output, repr(e))
        sys.exit(1)
    else:
      out_file = sys.stdout

    process(in_file, out_file)

    out_file.close()
    in_file.close()
    sys.exit(0)

if __name__ == '__main__':
    main()

