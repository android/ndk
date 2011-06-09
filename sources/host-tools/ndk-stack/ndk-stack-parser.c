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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include "elff/elff_api.h"

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
static const char _frame_header[] = "\#[0-9]{2}[ |\t]+[pc|eip]*[ |\t]+([0-9a-f]{8})*[ |\t]";

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
static int MatchRegex(const char* line, const char* regex, regmatch_t* match);

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

NdkCrashParser*
CreateNdkCrashParser(FILE* out_handle, const char* sym_root)
{
  NdkCrashParser* parser = (NdkCrashParser*)malloc(sizeof(NdkCrashParser));
  if (parser != NULL) {
    parser->out_handle = out_handle;
    parser->state = EXPECTS_CRASH_DUMP;
    parser->sym_root = (char*)malloc(strlen(sym_root) + 1);
    if (parser->sym_root != NULL) {
      strcpy(parser->sym_root, sym_root);
    } else {
      free(parser);
      parser = NULL;
    }
  }

  return parser;
}

void
DestroyNdkCrashParser(NdkCrashParser* parser)
{
  if (parser != NULL) {
    if (parser->sym_root != NULL) {
      free(parser->sym_root);
    }
    free(parser);
  }
}

int
ParseLine(NdkCrashParser* parser, const char* line)
{
  regmatch_t match;

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
      }
      // Let it fall through to the EXPECTS_PID, in case the dump doesn't
      // contain build fingerprint.
    case EXPECTS_PID:
      if (MatchRegex(line, _pid_header, &match)) {
        fprintf(parser->out_handle, "%s\n", line + match.rm_so);
        parser->state = EXPECTS_SIGNAL_OR_FRAME;
        return 0;
      } else {
        return 1;
      }

    case EXPECTS_SIGNAL_OR_FRAME:
      if (MatchRegex(line, _sig_header, &match)) {
        fprintf(parser->out_handle, "%s\n", line + match.rm_so);
        parser->state = EXPECTS_FRAME;
      }
      // Let it fall through to the EXPECTS_FRAME, in case the dump doesn't
      // contain signal fingerprint.
    case EXPECTS_FRAME:
      if (MatchRegex(line, _frame_header, &match)) {
        parser->state = EXPECTS_FRAME;
        return ParseFrame(parser, line + match.rm_so);
      } else {
        return 1;
      }

    default:
      return 1;
  }
}

static int
MatchRegex(const char* line, const char* regex, regmatch_t* match)
{
  char rerr[4096];
  regex_t rex;
  int ok = regcomp(&rex, regex, REG_ICASE);
  if (!ok) {
    ok = regexec(&rex, line, 1, match, REG_NOTEOL);
#if 0
    if (ok) {
        regerror(ok, &rex, rerr, sizeof(rerr));
        fprintf(stderr, "regexec(%s, %s) has failed: %s\n", line, regex, rerr);
    }
#endif
    regfree(&rex);
  } else {
    regerror(ok, &rex, rerr, sizeof(rerr));
    fprintf(stderr, "regcomp(%s) has failed: %s\n", regex, rerr);
  }

  return ok == 0;
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
    const size_t to_copy = min((end - start), (size - 1));
    memcpy(token, start, to_copy);
    token[to_copy] = '\0';
    return start;
  } else {
    return NULL;
  }
}

int
ParseFrame(NdkCrashParser* parser, const char* frame)
{
  uint64_t address;
  const char* wrk;
  char* eptr;
  char pc_address[17];
  char module_path[2048];
  char sym_file[2048];
  ELFF_HANDLE elff_handle;
  Elf_AddressInfo pc_info;

  fprintf(parser->out_handle, "Stack frame %s: ", frame);

  // Advance to the instruction pointer token.
  wrk = strstr(frame, "pc");
  if (wrk == NULL) {
    wrk = strstr(frame, "eip");
    if (wrk == NULL) {
      wrk = strstr(frame, "ip");
      if (wrk == NULL) {
        fprintf(parser->out_handle,
                "Parser is unable to locate instruction pointer token.\n");
        return -1;
      }
    }
  }

  // Next token after the instruction pointer token is its address.
  wrk = get_next_token(wrk, pc_address, sizeof(pc_address));
  // PC address is a hex value. Get it.
  eptr = pc_address + strlen(pc_address);
  address = strtoul(pc_address, &eptr, 16);

  // Next token is module path.
  get_next_token(wrk, module_path, sizeof(module_path));

  // Build path to the symbol file.
  strcpy(sym_file, parser->sym_root);
  strcat(sym_file, module_path);

  // Init ELFF wrapper for the symbol file.
  elff_handle = elff_init(sym_file);
  if (elff_handle == NULL) {
    fprintf(parser->out_handle,
            "Unable to open symbol file %s. Error code is %d\n",
            sym_file, errno);
    return -1;
  }
  // Extract address info from the symbol file.
  if (!elff_get_pc_address_info(elff_handle, address, &pc_info)) {
    if (pc_info.dir_name != NULL) {
      fprintf(parser->out_handle, "Routine %s in %s/%s:%d\n",
              pc_info.routine_name, pc_info.dir_name, pc_info.file_name,
              pc_info.line_number);
    } else {
      fprintf(parser->out_handle, "Routine %s in %s:%d\n",
              pc_info.routine_name, pc_info.file_name, pc_info.line_number);
    }
    elff_free_pc_address_info(elff_handle, &pc_info);
    elff_close(elff_handle);
    return 0;
  } else {
    fprintf(parser->out_handle,
            "Unable to locate routine information for address %x in module %s\n",
            (uint32_t)address, sym_file);
    elff_close(elff_handle);
    return -1;
  }
}
