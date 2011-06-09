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
 * Contains implementation of routines that implement platform-independent
 * file I/O.
 */

#include "stddef.h"
#include "sys/types.h"
#include "errno.h"
#ifdef  WIN32
#include "windows.h"
#else   // WIN32
#include <sys/mman.h>
#endif  // WIN32
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mapfile.h"

MapFile*
mapfile_open(const char* path, int oflag, int share_mode)
{
#ifdef WIN32
    DWORD win32_share;
    DWORD win32_desired_access = GENERIC_READ;
    DWORD win32_disposition = OPEN_EXISTING;
    DWORD win32_flags;

    /* Convert to Win32 desired access. */
    if ((oflag & O_RDWR) == O_RDWR) {
        win32_desired_access = GENERIC_READ | GENERIC_WRITE;
    } else if ((oflag & O_ACCMODE) == O_RDONLY) {
        win32_desired_access = GENERIC_READ;
    } else if ((oflag & O_WRONLY) == O_WRONLY) {
        win32_desired_access = GENERIC_WRITE;
    }

    /* Convert to Win32 sharing. */
    win32_share = 0;
    if ((share_mode & S_IWRITE) != 0) {
        win32_share |= FILE_SHARE_WRITE;
    }
    if ((share_mode & S_IREAD) != 0) {
        win32_share |= FILE_SHARE_READ;
    }

    /* Convert to Win32 disposition. */
    if ((oflag & O_CREAT) == O_CREAT) {
        if ((oflag & O_EXCL) == O_EXCL) {
            win32_disposition = CREATE_NEW;
        } else {
            win32_disposition = OPEN_ALWAYS;
        }
    } if ((oflag & O_TRUNC) == O_TRUNC) {
        win32_desired_access = TRUNCATE_EXISTING;
    } else {
        win32_disposition = OPEN_EXISTING;
    }

    /* Convert to Win32 flags. */
    win32_flags = 0;
#if defined(O_DSYNC)
    if ((oflag & O_DSYNC) == O_DSYNC ||
        (oflag & O_RSYNC) == O_RSYNC ||
        (oflag & O_RSYNC) == O_SYNC) {
        win32_flags |= FILE_FLAG_WRITE_THROUGH;
    }
#endif  // O_DSYNC

    HANDLE file_handle = CreateFile(path, win32_desired_access, win32_share,
                                    NULL, win32_disposition, win32_flags, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        errno = GetLastError();
    }
#else   // WIN32
    int file_handle = open(path, oflag, share_mode);
#endif  // WIN32

    return (MapFile*)(ptrdiff_t)file_handle;
}

int
mapfile_close(MapFile* handle)
{
#ifdef WIN32
    if (CloseHandle(handle)) {
        return 0;
    } else {
        errno = GetLastError();
        return -1;
    }
#else   // WIN32
    return close((int)(ptrdiff_t)handle);
#endif  // WIN32
}

ssize_t
mapfile_read(MapFile* handle, void* buf, size_t nbyte)
{
#ifdef WIN32
    ssize_t ret_bytes;
    DWORD read_bytes;
    if (ReadFile(handle, buf, nbyte, &read_bytes, NULL)) {
        ret_bytes = (ssize_t)read_bytes;
    } else {
        errno = GetLastError();
        ret_bytes = -1;
    }
    return ret_bytes;
#else   // WIN32
    ssize_t ret;
    do {
        ret = read((int)(ptrdiff_t)handle, buf, nbyte);
    } while (ret < 0 && errno == EINTR);
    return ret;
#endif  // WIN32
}

ssize_t
mapfile_read_at(MapFile* handle, size_t offset, void* buf, size_t nbyte)
{
#ifdef WIN32
    LARGE_INTEGER convert;
    convert.QuadPart = offset;
    if ((SetFilePointer(handle, convert.LowPart, &convert.HighPart,
                        FILE_BEGIN) == INVALID_SET_FILE_POINTER) &&
            (GetLastError() != NO_ERROR)) {
        errno = GetLastError();
        return -1;
    }
    return mapfile_read(handle, buf, nbyte);
#else   // WIN32
    ssize_t res = lseek((int)(ptrdiff_t)handle, offset, SEEK_SET);
    return res >= 0 ? mapfile_read(handle, buf, nbyte) : res;
#endif  // WIN32
}

void*
mapfile_map(MapFile* handle,
            size_t offset,
            size_t size,
            int prot,
            void** mapped_offset,
            size_t* mapped_size)
{
    void* mapped_at = NULL;
    size_t align_mask;
    size_t map_offset;
    size_t map_size;

  /* Get the mask for mapping offset alignment. */
#ifdef  WIN32
    DWORD win32_prot;
    DWORD win32_map;
    HANDLE map_handle;
    LARGE_INTEGER converter;
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    align_mask = sys_info.dwAllocationGranularity - 1;
#else   // WIN32
    align_mask = getpagesize() - 1;
#endif  // WIN32

    /* Adjust mapping offset and mapping size accordingly to
     * the mapping alignment requirements. */
    map_offset = offset & ~align_mask;
    map_size = (size_t)(offset - map_offset + size);

    /* Make sure mapping size doesn't exceed 4G. */
    if (map_size < size) {
        errno = EFBIG;
        return NULL;
    }

    /* Map the section. */
#ifdef  WIN32
    /* Convert to Win32 page protection and mapping type. */
    win32_prot = PAGE_READONLY;
    win32_map = FILE_MAP_READ;
    if (prot != PROT_NONE) {
        if ((prot & (PROT_WRITE | PROT_EXEC)) == 0) {
            win32_prot = PAGE_READONLY;
            win32_map = FILE_MAP_READ;
        } else if ((prot & (PROT_WRITE | PROT_EXEC)) ==
                   (PROT_WRITE | PROT_EXEC)) {
            win32_prot = PAGE_EXECUTE_READWRITE;
            win32_map = FILE_MAP_WRITE;
        } else if ((prot & PROT_WRITE) == PROT_WRITE) {
            win32_prot = PAGE_READWRITE;
            win32_map = FILE_MAP_WRITE;
        } else if ((prot & PROT_EXEC) == PROT_EXEC) {
            win32_prot = PAGE_EXECUTE_READ;
            win32_map = FILE_MAP_READ;
        }
    }

    converter.QuadPart = map_offset + map_size;
    map_handle = CreateFileMapping(handle, NULL, win32_prot,
                                   converter.HighPart, converter.LowPart, NULL);
    if (map_handle != NULL) {
        converter.QuadPart = map_offset;
        mapped_at = MapViewOfFile(map_handle, win32_map, converter.HighPart,
                                  converter.LowPart, map_size);
        /* Memory mapping (if successful) will hold extra references to the
        * mapping, so we can close it right after we mapped file view. */
        CloseHandle(map_handle);
    }
    if (mapped_at == NULL) {
        errno = GetLastError();
        return NULL;
    }
#else   // WIN32
    mapped_at =
        mmap(0, map_size, PROT_READ, MAP_SHARED, (int)(ptrdiff_t)handle, map_offset);
    if (mapped_at == MAP_FAILED) {
        return NULL;
    }
#endif  // WIN32

    *mapped_offset = (char*)mapped_at + (offset - map_offset);
    *mapped_size = size;

    return mapped_at;
}

int
mapfile_unmap(void* mapped_at, size_t len)
{
#ifdef WIN32
    if (!UnmapViewOfFile(mapped_at)) {
        errno = GetLastError();
        return -1;
    }
    return 0;
#else   // WIN32
    return munmap(mapped_at, len);
#endif  // WIN32
}
