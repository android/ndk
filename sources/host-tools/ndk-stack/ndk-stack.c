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
 * Contains implementation of main routine for ndk-stack utility.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ndk-stack-parser.h"

/* Usage string. */
static const char* _usage_str =
"Usage:\n"
"   ndk-stack -sym <path> [-dump <path>]\n\n"
"      -sym  Contains full path to the root directory for symbols.\n"
"      -dump Contains full path to the file containing the crash dump.\n"
"            This is an optional parameter. If ommited, ndk-stack will\n"
"            read input data from stdin\n"
"\n"
"   See docs/NDK-STACK.html in your NDK installation tree for more details.\n\n";

int main(int argc, char **argv, char **envp)
{
    const char* dump_file = NULL;
    const char* sym_path = NULL;
    int use_stdin = 0;

    /* Parse command line. */
    {
        int n;
        for (n = 1; n < argc; n++) {
            if (!strcmp(argv[n], "-dump")) {
                n++;
                if (n < argc) {
                    dump_file = argv[n];
                }
            } else if (!strcmp(argv[n], "-sym")) {
                n++;
                if (n < argc) {
                    sym_path = argv[n];
                }
            } else {
                fprintf(stdout, "%s", _usage_str);
                return -1;
            }
        }
        if (sym_path == NULL) {
            fprintf(stdout, "%s", _usage_str);
            return -1;
        }
        if (dump_file == NULL) {
            use_stdin = 1;
        }
    }

    /* Create crash dump parser, open dump file, and parse it line by line. */
    NdkCrashParser* parser = CreateNdkCrashParser(stdout, sym_path);
    if (parser != NULL) {
        FILE* handle = use_stdin ? stdin : fopen(dump_file, "r");
        if (handle != NULL) {
            char str[2048];
            while (fgets(str, sizeof(str), handle)) {
                /* ParseLine requires that there are no \r, or \n symbols in the
                 * string. */
                str[strcspn(str, "\r\n")] = '\0';
                ParseLine(parser, str);
            }
            fclose(handle);
        } else {
            fprintf(stderr, "Unable to open dump file %s: %s\n",
                    dump_file, strerror(errno));
        }
        DestroyNdkCrashParser(parser);
    } else {
        fprintf(stderr, "Unable to create NDK stack parser: %s\n",
                strerror(errno));
    }
    return 0;
}

