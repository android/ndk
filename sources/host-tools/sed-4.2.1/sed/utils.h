/*  Functions from hack's utils library.
    Copyright (C) 1989, 1990, 1991, 1998, 1999, 2003
    Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdio.h>

#include "basicdefs.h"

void panic P_((const char *str, ...));

FILE *ck_fopen P_((const char *name, const char *mode, int fail));
FILE *ck_fdopen P_((int fd, const char *name, const char *mode, int fail));
void ck_fwrite P_((const VOID *ptr, size_t size, size_t nmemb, FILE *stream));
size_t ck_fread P_((VOID *ptr, size_t size, size_t nmemb, FILE *stream));
void ck_fflush P_((FILE *stream));
void ck_fclose P_((FILE *stream));
const char *follow_symlink P_((const char *path));
size_t ck_getline P_((char **text, size_t *buflen, FILE *stream));
FILE * ck_mkstemp P_((char **p_filename, char *tmpdir, char *base));
void ck_rename P_((const char *from, const char *to, const char *unlink_if_fail));

VOID *ck_malloc P_((size_t size));
VOID *xmalloc P_((size_t size));
VOID *ck_realloc P_((VOID *ptr, size_t size));
char *ck_strdup P_((const char *str));
VOID *ck_memdup P_((const VOID *buf, size_t len));
void ck_free P_((VOID *ptr));

struct buffer *init_buffer P_((void));
char *get_buffer P_((struct buffer *b));
size_t size_buffer P_((struct buffer *b));
char *add_buffer P_((struct buffer *b, const char *p, size_t n));
char *add1_buffer P_((struct buffer *b, int ch));
void free_buffer P_((struct buffer *b));

extern const char *myname;
