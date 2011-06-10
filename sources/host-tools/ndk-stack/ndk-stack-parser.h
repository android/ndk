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

#ifndef NDK_CRASH_PARSER_H_
#define NDK_CRASH_PARSER_H_

/*
 * Contains declaration of structures and routines that are used to parse ADB
 * log output, filtering out and printing references related to the crash dump.
 */

/* Crash parser descriptor. */
typedef struct NdkCrashParser NdkCrashParser;

/* Creates and initializes NdkCrashParser descriptor.
 * Param:
 *  out_handle - Handle to the stream where to print the parser's output.
 *    Typically, the handle is is stdout.
 *  sym_root - Path to the root directory where symbols are stored. Note that
 *    symbol tree starting with that root must match the tree of execuatable
 *    modules in the device. I.e. symbols for /path/to/module must be located in
 *    <sym_root>/path/to/module
 * Return:
 *  Pointer to the initialized NdkCrashParser descriptor on success, or NULL on
 *  failure.
 */
NdkCrashParser* CreateNdkCrashParser(FILE* out_handle, const char* sym_root);

/* Destroys an NdkCrashParser descriptor.
 * Param:
 *  parser - NdkCrashParser descriptor, created and initialized with a call to
 *    NdkCrashParser routine.
 */
void DestroyNdkCrashParser(NdkCrashParser* parser);

/* Parses a line from the ADB log output.
 * Param:
 *  parser - NdkCrashParser descriptor, created and initialized with a call to
 *    NdkCrashParser routine.
 *  line - ADB log output line to parse. The line must be zero-terminated, and
 *    must not contain \r, or \n in it.
 * Return:
 *  0 If the line has been part of the crash dump.
 *  1 If the line has not been part of the crash dump.
 *  -1 If there was an error when parsing the line.
 *
*/
int ParseLine(NdkCrashParser* parser, const char* line);

#endif  // NDK_CRASH_PARSER_H_
