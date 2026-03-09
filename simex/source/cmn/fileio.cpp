/*
**  sagetools
**  Copyright 2026 Jonathan Wilson
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cmn\fileio.h"
#include <algorithm>

int (*gFileIOprintf)(const char*, ...) = FileIOprintf;
int FileIO::sOpenMethod = 3;

int FileIOprintf(const char* format, ...)
{
    va_list arglist;
    int r;
    char tempstr[512];

    va_start(arglist, format);
    r = vsprintf(tempstr, format, arglist);
    printf("%s", tempstr);
    va_end(arglist);
    return r;
}

__int64 FileIO::WriteFlush(FileIOHandle* h)
{
    __int64 bytes = 0;

    if (h->dirty && h->write)
    {
        fseek(h->handle, (int)h->cachestart, SEEK_SET);

        if ((h->bufend - h->buf) > 0)
        {
            bytes = fwrite((void*)h->buf, (size_t)1, (size_t)(h->bufend - h->buf), h->handle);
        }

        if (bytes < (h->bufend - h->buf))
        {
            gFileIOprintf("WriteFlush - OUT OF DISK SPACE\n");
        }

        h->dirty = 0;
    }

    h->bufptr = h->bufend = h->buf;
    fflush(h->handle);

    return bytes;
}

void FileIO::MakeTempName(char* d, const char* s, const char* ext)
{
    strcpy(d, s);
    strcat(d, ext);
}

int FileIO::IsReadOnly(const char* filename)
{
    struct _stat buf;

    if (_stat(filename, &buf) != -1)
    {
        if ((buf.st_mode & S_IREAD) && !(buf.st_mode & S_IWRITE))
        {
            return 1;
        }
    }

    return 0;
}

int FileIO::IsDirectory(const char* filename)
{
    struct _stat buf;

    if (_stat(filename, &buf) != -1)
    {
        if (buf.st_mode & S_IFDIR)
        {
            return 1;
        }
    }

    return 0;
}

void* Allocator::Alloc(int size)
{
    char* memptr;

    if (size <= 0)
    {
        gFileIOprintf("Allocator::Alloc - INVALID SIZE %ld\n", size);
        return (void*)0;
    }

    memptr = (char*)malloc((size_t)size + 12);

    if (!memptr || memptr == (char*)-1)
    {
        gFileIOprintf("Allocator::Alloc - FAILED. OUT OF MEMORY. SIZE REQUESTED %ld\n", size);
        return (void*)0;
    }

    memcpy(memptr, "GMEM", 4);
    *(int*)((char*)memptr + 4) = size;
    memcpy(memptr + 8 + size, "GEND", 4);
    return (void*)(memptr + 8);
}

int Allocator::Free(void* memptr)
{
    int size;
    int ok = 0;

    if ((int)memptr > -2048 && (int)memptr < 2048)
    {
        gFileIOprintf("Allocator::Free - ATTEMPT TO FREE NULL POINTER\n");
    }
    else
    {
        size = *(int*)((char*)memptr - 4);

        if (!memcmp(((char*)memptr - 8), "GMEM", 4) && size > 0)
        {
            if (!memcmp(((char*)memptr + size), "GEND", 4))
            {
                ok = 1;
            }
        }

        if (!ok)
        {
            if (!memcmp((char*)memptr - 8, "GFRE", 4))
            {
                gFileIOprintf("Allocator::Free - ATTEMPT TO FREE MEMORY TWICE\n");
            }
            else
            {
                gFileIOprintf("Allocator::Free - SENTINAL CORRUPTED; PROBABLE MEMORY DATA CORRUPTION\n");
            }
        }
        else
        {
            memcpy((char*)memptr - 8, "GFRE", 4);
            free((char*)memptr - 8);
        }
    }

    return ok;
}

unsigned int GetM(const void* src, int bytes)
{
    unsigned char* s = (unsigned char*)src;
    unsigned int value;
    value = 0L;

    while (bytes--)
    {
        value = (value << 8) + ((*s++));
    }

    return value;
}

unsigned int GetI(const void* src, int bytes)
{
    unsigned char* s = (unsigned char*)src;
    int i = 0;
    unsigned int value;
    value = 0L;

    while (bytes--)
    {
        value += ((*s++)) << (i);
        i += 8;
    }

    return value;
}

void PutM(void* dst, unsigned int data, int bytes)
{
    unsigned char* d = (unsigned char*)dst;
    unsigned int pval;
    data <<= (4 - bytes) * 8;

    while (bytes)
    {
        pval = data >> 24;
        *d++ = (unsigned char)pval;
        data <<= 8;
        bytes--;
    }
}

void PutI(void* dst, unsigned int data, int bytes)
{
    unsigned char* d = (unsigned char*)dst;
    unsigned int pval;

    while (bytes)
    {
        pval = data;
        *d++ = (unsigned char)pval;
        data >>= 8;
        bytes--;
    }
}

FileHandle* FileIO::Open(const char* filename)
{
    FILE* handle;
    int len = 0;
    FileIOHandle* h;
    int gmethod;

    if (!strncmp(filename, "con:", 4))
    {
        handle = stdin;
        gmethod = 0;
    }
    else
    {
        handle = fopen(filename, "rb");
        gmethod = 1;
    }

    h = 0;

    if (handle)
    {
        int ok;
        ok = !fseek(handle, 0, SEEK_END);

        if (ok)
        {
            len = ftell(handle);
            fseek(handle, 0, SEEK_SET);

            h = (FileIOHandle*)Allocator::Alloc(sizeof(FileIOHandle));

            if (h)
            {
                h->handle = handle;
                h->offset = 0;
                h->len = len;
                h->bufptr = h->buf;
                h->bufend = h->buf;
                h->write = 0;
                h->dirty = 0;
                h->gopenmethod = gmethod;
                h->cachestart = 0;
                strcpy(h->filename, filename);
            }
            else
            {
                gFileIOprintf("gopen - UNABLE TO ALLOCATE MEMORY FOR FILE HANDLE (%s)\n", filename);
            }
        }
        else
        {
            gFileIOprintf("gopen - SEEK ON OPEN FAILED (%s)\n", filename);
        }
    }

    return (FileHandle*)h;
}

FileHandle* FileIO::WOpen(const char* filename)
{
    FILE* handle = 0;
    int len;
    FileIOHandle* h = 0;
    int gmethod = sOpenMethod;
    char tempname[1024];
    tempname[0] = (char)0;

    if (!strncmp(filename, "con:", 4))
    {
        handle = stdout;
        gmethod = 0;
    }
    else
    {
        if (IsReadOnly(filename))
        {
            gFileIOprintf("gwopen - ATTEMPT TO OPEN READONLY FILE FOR WRITING (%s)\n", filename);
        }
        else
        {
            if (IsDirectory(filename))
            {
                gFileIOprintf("gwopen - ATTEMPT TO OPEN DIRECTORY FOR WRITING (%s)\n", filename);
            }
            else
            {
                if (gmethod == 2 || gmethod == 3)
                {
                    MakeTempName(tempname, filename, ".tm1");
                }
                else
                {
                    strcpy(tempname, filename);
                }

                handle = fopen(tempname, "w+b");

                if (!handle)
                {
                    handle = fopen(tempname, "wb");
                    gFileIOprintf("gwopen - UNABLE TO OPEN FILE FOR WRITE (%s)\n", tempname);
                }
            }
        }
    }

    len = 0;

    if (handle)
    {
        h = (FileIOHandle*)Allocator::Alloc(sizeof(FileIOHandle));

        if (h)
        {
            h->handle = handle;
            h->offset = 0;
            h->len = len;
            h->bufptr = h->buf;
            h->bufend = h->buf;
            h->write = 1;
            h->dirty = 0;
            h->gopenmethod = gmethod;
            h->cachestart = 0;
            strcpy(h->filename, filename);
            strcpy(h->tempname, tempname);
        }
        else
        {
            gFileIOprintf("gwopen - UNABLE TO ALLOCATE MEMORY FOR FILE HANDLE (%s)\n", filename);
        }
    }

    return (FileHandle*)h;
}

int FileIO::Close(FileHandle* g)
{
    FileIOHandle* h = (FileIOHandle*)g;
    int ok = 1;

    if (g)
    {
        WriteFlush(h);

        {
            int gmethod = h->gopenmethod;

            if (gmethod)
            {
                ok = !fclose(h->handle);

                if (ok && (gmethod == 2 || gmethod == 3))
                {
                    char tm2name[1024];
                    MakeTempName(tm2name, h->filename, ".tm2");
                    remove(tm2name);
                    rename(h->filename, tm2name);
                    ok = !rename(h->tempname, h->filename);

                    if (ok)
                    {
                        if (gmethod == 2)
                        {
                            char bakname[1024];
                            MakeTempName(bakname, h->filename, ".bak");
                            remove(bakname);
                            rename(tm2name, bakname);
                        }
                        else
                        {
                            remove(tm2name);
                        }
                    }
                }
            }

            ok &= Allocator::Free((char*)g);
        }
    }

    return ok;
}

int FileIO::Read(FileHandle* g, void* buf, int size)
{
    FileIOHandle* h = ((FileIOHandle*)g);
    int len;
    int bytes = 0, bytes2 = 0;

    if (g == NULL)
    {
        return 0;
    }

    if (size < 0)
    {
        size = 0;
    }

    len = std::min(static_cast<int>(size), static_cast<int>(h->bufend - h->bufptr));

    if (len > 0)
    {
        memcpy(buf, h->bufptr, (size_t)len);
        h->bufptr += len;
        h->offset += len;
        buf = (void*)((char*)buf + len);
        size -= len;
        bytes += len;
    }

    size = (int)std::min(static_cast<int>(size), static_cast<int>(h->len - h->offset));

    if (size < 0)
    {
        gFileIOprintf("gread - ATTEMPT TO READ PAST END OF FILE\n");
        return bytes;
    }

    if (!(h->bufend - h->bufptr) && size)
    {
        int blockstart = ((int)((h->offset + size) / 8192u)) * 8192;
        WriteFlush(h);

        if (h->offset < blockstart)
        {
            bytes2 = (int)fread((void*)buf, (size_t)1, (size_t)(blockstart - h->offset), h->handle);
            buf = (void*)((char*)buf + bytes2);
            size -= bytes2;
        }

        h->cachestart = blockstart;
        len = (int)std::min(8192, static_cast<int>(h->len - blockstart));
        fread((void*)h->buf, (size_t)1, (size_t)len, h->handle);
        memcpy(buf, h->bufptr, (size_t)size);
        bytes2 += size;
        h->bufptr += size;
        h->bufend += len;
        h->offset += bytes2;
    }

    return bytes + bytes2;
}

int FileIO::Write(FileHandle* g, const void* buf, int size)
{
    FileIOHandle* h = ((FileIOHandle*)g);
    int len;
    int bytes = 0;

    if (g == NULL)
    {
        return 0;
    }

    if (!h->write)
    {
        gFileIOprintf("gwrite - ATTEMPT TO WRITE TO A READ ONLY FILE\n");
        return 0;
    }

    len = std::min(static_cast<int>(size), static_cast<int>(h->buf + 8192 - h->bufptr));

    if (len > 0)
    {
        memcpy((void*)h->bufptr, (void*)buf, (size_t)len);
        h->bufptr += len;
        h->offset += len;

        if (h->bufend < h->bufptr)
        {
            h->bufend = h->bufptr;
        }

        if (h->offset > h->len)
        {
            h->len = h->offset;
        }

        h->dirty = 1;
        buf = (void*)((char*)buf + len);
        size -= len;
        bytes += len;
    }

    if (!(h->bufend - h->bufptr) && size)
    {
        int blockstart = ((int)((h->offset + size) / 8192u)) * 8192;
        WriteFlush(h);

        if (h->len > blockstart)
        {
            len = (int)std::min(8192, static_cast<int>(h->len - blockstart));
            fseek(h->handle, blockstart, SEEK_SET);
            fread((void*)h->buf, (size_t)1, (size_t)len, h->handle);
            h->bufend += len;
        }

        fseek(h->handle, (int)h->offset, SEEK_SET);
        len = 0;

        if ((blockstart - h->offset) > 0)
        {
            len = (int)fwrite((void*)buf, (size_t)1, (size_t)(blockstart - h->offset), h->handle);
        }

        if (len < (blockstart - h->offset))
        {
            gFileIOprintf("gwrite - OUT OF DISK SPACE\n");
            return bytes + len;
        }

        h->offset += len;
        bytes += size;
        size -= len;
        memcpy((void*)h->buf, (void*)((char*)buf + len), (size_t)size);
        h->offset += size;
        h->cachestart = blockstart;
        h->bufptr += size;

        if (h->offset > h->len)
        {
            h->len = h->offset;
        }

        if (h->bufptr > h->bufend)
        {
            h->bufend = h->bufptr;
        }

        h->dirty = 1;
    }

    return bytes;
}

int FileIO::Seek(FileHandle* g, __int64 offset)
{
    FileIOHandle* h = ((FileIOHandle*)g);
    __int64 relative;
    int ok = 1;
    int len;

    if (g == NULL)
    {
        return 0;
    }

    if (offset < 0)
    {
        offset = 0;
        ok = 0;
    }
    else if (offset > h->len && !h->write)
    {
        offset = h->len;
        ok = 0;
    }

    relative = (__int64)(offset - h->offset);
    h->offset += (long)relative;

    if ((relative < h->buf - h->bufptr) || (relative > h->bufend - h->bufptr))
    {
        int blockstart = ((int)(h->offset / 8192u)) * 8192;
        WriteFlush(h);
        ok = !fseek(h->handle, blockstart, SEEK_SET) && ok;
        h->cachestart = blockstart;

        if (h->len > blockstart)
        {
            len = (int)std::min(8192, static_cast<int>(h->len - blockstart));
            fread((void*)h->buf, (size_t)1, (size_t)len, h->handle);
            h->bufend += len;
            h->bufptr += (h->offset - blockstart);
        }
        else
        {
            len = (int)h->offset - blockstart;
            memset((void*)h->buf, (unsigned char)0x00, (size_t)len);
            h->bufptr = h->bufend = h->buf + len;
        }
    }
    else
    {
        h->bufptr += relative;
    }

    return ok;
}

__int64 FileIO::Length(FileHandle* g)
{
    return ((FileIOHandle*)g)->len;
}

__int64 FileIO::Tell(FileHandle* g)
{
    return ((FileIOHandle*)g)->offset;
}
