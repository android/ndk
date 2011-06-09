/* Copyright (C) 2007-2010 The Android Open Source Project
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
 * Contains declarations of routines that implement platform-independent
 * file I/O.
 */

#ifndef _ANDROID_UTILS_FILEIO_H
#define _ANDROID_UTILS_FILEIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MapFile MapFile;

#ifdef WIN32
/* Declare constants that are missing in Win32 headers. */
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_NONE   0x0
#endif

/* Checks if file handle is a valid one.
 * Return:
 *  boolean: 1 if handle is valid, or 0 if it's not valid.
 */
static inline int
mapfile_is_valid(MapFile* handle)
{
    return handle != (void*)(ptrdiff_t)-1;
}

/* Opens file in selected mode.
 * Param:
 *  path - Path to the file to open.
 *  oflag - Defines mode in which file is to be opened. This value follows the
 *      symantics of O_XXX flags defined for standard open routine.
 *  share_mode Defines sharing mode for the opened file. This value follows the
 *      symantics of S_IXXX flags defined for standard open routine.
 * Return:
 *  A valid handle to the opened file on success, or an invalid value on
 *  failure. In case of failure errno contains error code.
 */
extern MapFile* mapfile_open(const char* path, int oflag, int share_mode);

/* Closes a file handle opened with mapfile_open routine.
 * Param:
 *  handle - A handle to a file previously obtained via successful call to
 *      mapfile_open routine.
 * Return:
 *  0 on success, or -1 on failure with errno containing the error code.
 */
extern int mapfile_close(MapFile* handle);

/* Reads from a file opened with mapfile_open routine.
 * Except for handle parameter, semantics of this call are the same as for
 * the regualar read routine.
 * Param:
 *  handle - A handle to a file previously obtained via successful call to
 *      mapfile_open routine.
 */
extern ssize_t mapfile_read(MapFile* handle, void* buf, size_t nbyte);

/* Reads from a specific offset in a file opened with mapfile_open routine.
 * Param:
 *  handle - A handle to a file previously obtained via successful call to
 *      mapfile_open routine.
 *  offset - Offset in the file where to start reading from.
 *  Rest of the parameters and return value are the same as in file_read.
 */
extern ssize_t mapfile_read_at(MapFile* handle,
                               size_t offset,
                               void* buf,
                               size_t nbyte);

/* Maps a section of a file to memory.
 * Param:
 *  handle - A handle to a file previously obtained via successful call to
 *      mapfile_open routine.
 *  offset - Offset in the file where mapping should begin.
 *  size - Number of bytes starting with offset that should be mapped.
 *  prot - Determines whether read, write, execute, or some combination of
 *      accesses are permitted to the data being mapped. This parameter has the
 *      same semantics as in regular mmap routene.
 *  mapped_offset - Upon success, contains pointer to the requested offset
 *      within the mapped section of the file.
 *  size - Upon success, contains total number of bytes that were actually
 *      mapped.
 * Return:
 *  Upon successful completion returns pointer to the beginning of memory
 *  mapping, containing mapping of the requested section of a file. Note that
 *  value returned from this routine doesn't necessarily points to the beginning
 *  of the requested section mapping. Use value returned in mapped_offset
 *  parameter to get actual pointer to the beginning of the requested section
 *  mapping. Value returned from this routine must eventually be passed to
 *  file_unmap_section reoutine to unmap section mapped with this routine.
 *  This routine returns NULL on failure and sets errno to indicate the error.
 */
extern void* mapfile_map(MapFile* handle,
                         size_t offset,
                         size_t size,
                         int prot,
                         void** mapped_offset,
                         size_t* mapped_size);

/* Umaps section of a file previously mapped with mapfile_map routine.
 * Param:
 *  mapped_at - A pointer to the base address of the mapped section of a file
 *      that is to be unmapped.
 *  len - Byte size of the section that is to be unmapped.
 * Return:
 *  Upon successful completion returns 0. Otherwise, returns -1 and sets
 *  errno to indicate the error.
 */
extern int mapfile_unmap(void* mapped_at, size_t len);

#ifdef __cplusplus
}   /* end of extern "C" */
#endif

#endif  // _ANDROID_UTILS_FILEIO_H
