/* Copyright (C) 2007-2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * Contains implementation of a class DumpFile of routines that implements
 * access to a log file.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "regex/regex.h"

#include "ndk-stack-parser.h"

/* Enumerates states of the crash parser.
 */
typedef enum NDK_CRASH_PARSER_STATE {
  /* Parser expects the beginning of the crash dump. */
  EXPECTS_CRASH_DUMP,
  /* Parser expects the build fingerprint, or process and thread information. */
  EXPECTS_BUILD_FINGREPRINT_OR_PID,
  /* Parser expects the process and thread information. */
  EXPECTS_PID,
  /* Parser expects the signal information, or the first crash frame. */
  EXPECTS_SIGNAL_OR_FRAME,
  /* Parser expects a crash frame. */
  EXPECTS_FRAME,
} NDK_CRASH_PARSER_STATE;

/* Crash parser descriptor.
 */
struct NdkCrashParser {
  /* Handle to the stream where to print the output. */
  FILE*                 out_handle;

  /* Path to the root folder where symbols are stored. */
  char*                 sym_root;

  /* Current state of the parser. */
  NDK_CRASH_PARSER_STATE state;

  /* Compiled regular expressions */
  regex_t     re_pid_header;
  regex_t     re_sig_header;
  regex_t     re_frame_header;
};

/* Crash dumps begin with a string containing this substring. */
static const char _crash_dump_header[] =
  "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***";

/* Build fingerprint contains this substring. */
static const char _build_fingerprint_header[] = "Build fingerprint:";

/* Regular expression for the process ID information line. */
static const char _pid_header[] = "pid: [0-9]+, tid: [0-9]+.*";

/* Regular expression for the signal information line. */
static const char _sig_header[] = "signal*[ \t][0-9]+";

/* Regular expression for the frame information line. */
static const char _frame_header[] = "\\#[0-9]+[ |\t]+[pc|eip]+:*[ |\t]+([0-9a-f]{8})*";

#ifndef min
#define min(a,b) (((a) < (b)) ? a : b)
#endif

/* Parses a line representing a crash frame.
 * This routine will try to obtain source file / line information for the
 * frame's address, and print that information to the specified output handle.
 * Param:
 *  parser - NdkCrashParser descriptor, created and initialized with a call to
 *    NdkCrashParser routine.
 *  frame - Line containing crash frame.
 * Return:
 *  0 If source file information has been found and printed, or -1 if that
 *  information was not available.
 */
static int ParseFrame(NdkCrashParser* parser, const char* frame);

/* Matches a string against a regular expression.
 * Param:
 *  line - String to matches against the regular expression.
 *  regex - Regular expression to match the string against.
 *  match - Upon successful match contains information about the part of the
 *    string that matches the regular expression.
 * Return:
 *  Boolean: 1 if a match has been found, or 0 if match has not been found in
 *  the string.
 */
static int MatchRegex(const char* line, const regex_t* regex, regmatch_t* match);

/* Returns pointer to the next separator (a space, or a tab) in the string. */
static const char* next_separator(const char* str);

/* Returns pointer to the next token (a character other than space, or a tab)
 * in the string.
 */
static const char* next_token(const char* str);

/* Gets next token from the string.
 * param:
 *  str - String where to get the next token from. Note that if string begins
 *    with a separator, this routine will return first token after that
 *    separator. If string begins with a token, this routine will return next
 *    token after that.
 *  token - Upon success contains a copy of the next token in the string.
 *  size - Size of the 'token' buffer.
 * Return:
 *  Beginning of the returned token in the string.
 */
static const char* get_next_token(const char* str, char* token, size_t size);

/* Return pointer to first word "pc", "eip", or "ip" in string "frame"
 * param:
 *  frame - a line from dump
 * Return:
 *  The first occurrence of "pc", "eip", or "ip"
 */
static const char* find_pc(const char *frame);

NdkCrashParser*
CreateNdkCrashParser(FILE* out_handle, const char* sym_root)
{
  NdkCrashParser* parser;

  parser = (NdkCrashParser*)calloc(sizeof(*parser), 1);
  if (parser == NULL)
      return NULL;

  parser->out_handle = out_handle;
  parser->state      = EXPECTS_CRASH_DUMP;

  parser->sym_root = strdup(sym_root);
  if (!parser->sym_root)
      goto BAD_INIT;

  if (regcomp(&parser->re_pid_header, _pid_header, REG_EXTENDED | REG_NEWLINE) ||
      regcomp(&parser->re_sig_header, _sig_header, REG_EXTENDED | REG_NEWLINE) ||
      regcomp(&parser->re_frame_header, _frame_header, REG_EXTENDED | REG_NEWLINE))
      goto BAD_INIT;

  return parser;

BAD_INIT:
  DestroyNdkCrashParser(parser);
  return NULL;
}

void
DestroyNdkCrashParser(NdkCrashParser* parser)
{
  if (parser != NULL) {
    /* Release compiled regular expressions */
    regfree(&parser->re_frame_header);
    regfree(&parser->re_sig_header);
    regfree(&parser->re_pid_header);
    /* Release symbol path */
    free(parser->sym_root);
    /* Release parser itself */
    free(parser);
  }
}

int
ParseLine(NdkCrashParser* parser, const char* line)
{
  regmatch_t match;
  int found = 0;

  if (line == NULL || *line == '\0') {
    // Nothing to parse.
    return 1;
  }

  // Lets see if this is the beginning of a crash dump.
  if (strstr(line, _crash_dump_header) != NULL) {
    if (parser->state != EXPECTS_CRASH_DUMP) {
      // Printing another crash dump was in progress. Mark the end of it.
      fprintf(parser->out_handle, "Crash dump is completed\n\n");
    }

    // New crash dump begins.
    fprintf(parser->out_handle, "********** Crash dump: **********\n");
    parser->state = EXPECTS_BUILD_FINGREPRINT_OR_PID;

    return 0;
  }

  switch (parser->state) {
    case EXPECTS_BUILD_FINGREPRINT_OR_PID:
      if (strstr(line, _build_fingerprint_header) != NULL) {
        fprintf(parser->out_handle, "%s\n",
                strstr(line, _build_fingerprint_header));
        parser->state = EXPECTS_PID;
        found = 1;
      }
      // Let it fall through to the EXPECTS_PID, in case the dump doesn't
      // contain build fingerprint.
    case EXPECTS_PID:
      if (MatchRegex(line, &parser->re_pid_header, &match)) {
        fprintf(parser->out_handle, "%s\n", line + match.rm_so);
        parser->state = EXPECTS_SIGNAL_OR_FRAME;
        return 0;
      } else {
        return !found;
      }

    case EXPECTS_SIGNAL_OR_FRAME:
      if (MatchRegex(line, &parser->re_sig_header, &match)) {
        fprintf(parser->out_handle, "%s\n", line + match.rm_so);
        parser->state = EXPECTS_FRAME;
        found = 1;
      }
      // Let it fall through to the EXPECTS_FRAME, in case the dump doesn't
      // contain signal fingerprint.
    case EXPECTS_FRAME:
      if (!MatchRegex(line, &parser->re_frame_header, &match))
        return !found;
      // Regex generated by x86_64-w64-mingw32 compiler erroneously match
      // frame line with #[0-9]+ in "stack:" section even when the line has
      //  no word "pc", "eip", or "ip" in it.
      //
      //   stack:
      //      I/DEBUG   ( 1151):     #00  5f09db68  401f01c4  /system/lib/libc.so
      //
      // To workaround, let's double check if pc is found!
      //
      if (!(find_pc(line)))
        return !found;

      parser->state = EXPECTS_FRAME;
      return ParseFrame(parser, line + match.rm_so);

    default:
      return 1;
  }
}

static int
MatchRegex(const char* line, const regex_t* regex, regmatch_t* match)
{
  int err = regexec(regex, line, 1, match, 0x00400/*REG_TRACE*/);
#if 0
  char rerr[4096];
  if (err) {
    regerror(err, regex, rerr, sizeof(rerr));
    fprintf(stderr, "regexec(%s, %s) has failed: %s\n", line, regex, rerr);
  }
#endif

  return err == 0;
}

static const char*
next_separator(const char* str)
{
  return str + strcspn(str, " \t");
}

static const char*
next_token(const char* str)
{
  str = next_separator(str);
  return str + strspn(str, " \t");
}

static const char*
get_next_token(const char* str, char* token, size_t size)
{
  const char* start = next_token(str);
  const char* end = next_separator(start);
  if (start != end) {
    const size_t to_copy = min((size_t)(end - start), (size - 1));
    memcpy(token, start, to_copy);
    token[to_copy] = '\0';
    return start;
  } else {
    return NULL;
  }
}

static const char *
find_pc(const char *frame)
{
  const char *pcstrs[] = { "pc", "eip", "ip" };
  int i;
  for (i=0; i<sizeof(pcstrs)/sizeof(pcstrs[0]); i++) {
    const char *p = strstr(frame, pcstrs[i]);
    // check it's a word, not part of filename or something
    if (p && p!=frame) {
      char l = p[-1];
      char r = p[strlen(pcstrs[i])];
      if ((l==' ' || l=='\t') && (r==' ' || r=='\t'))
        return p;
    }
  }
  return NULL;
}

int
ParseFrame(NdkCrashParser* parser, const char* frame)
{
  uint64_t address;
  const char* wrk;
  char* eptr;
  char pc_address[17];
  char module_path[2048];
  char* module_name;
  char sym_file[2048];
  const int ac = 5;
  char *av[ac];
  FILE *f;

  fprintf(parser->out_handle, "Stack frame %s", frame);

  // Advance to the instruction pointer token.
  if ((wrk=find_pc(frame)) == NULL) {
    fprintf(parser->out_handle,
            "Parser is unable to locate instruction pointer token.\n");
    return -1;
  }

  // Next token after the instruction pointer token is its address.
  wrk = get_next_token(wrk, pc_address, sizeof(pc_address));
  // PC address is a hex value. Get it.
  eptr = pc_address + strlen(pc_address);
  address = strtoul(pc_address, &eptr, 16);

  // Next token is module path.
  get_next_token(wrk, module_path, sizeof(module_path));

  // Extract basename of module, we should not care about its path
  // on the device.
  module_name = strrchr(module_path,'/');
  if (module_name == NULL)
      module_name = module_path;
  else {
      module_name += 1;
      if (*module_name == '\0') {
          /* Trailing slash in the module path, this should not happen */
          /* Back-off with the full module-path */
          module_name = module_path;
      }
  }

  // Build path to the symbol file.
  snprintf(sym_file, sizeof(sym_file), "%s/%s", parser->sym_root, module_name);

  if ((f=fopen(sym_file, "r")) == NULL) {
    if (errno == ENOENT) {
        printf("\n");
    } else {
        printf(": Unable to open symbol file %s. Error (%d): %s\n",
                sym_file, errno, strerror(errno));
    }
    return -1;
  }

  // call addr2line if sym_file exist
  extern int addr2line_main (int argc, char **argv);

  av[0] = "ndk-stack";
  av[1] = "-fpC";  // f:function, p:pretty-print, C:demangle
  av[2] = "-e";    // e:exe-filename
  av[3] = sym_file;
  av[4] = pc_address;
  (void)address;

  printf(": Routine ");
  return addr2line_main(ac, av);
}
