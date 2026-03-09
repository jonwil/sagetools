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

#ifndef FILEIO_H
#define FILEIO_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int(__cdecl* gprintf)(const char*, ...);
int FileIOprintf(const char* format, ...);
unsigned int GetM(const void* src, int bytes);
unsigned int GetI(const void* src, int bytes);
void PutM(void* dst, unsigned int data, int bytes);
void PutI(void* dst, unsigned int data, int bytes);

struct FileHandle
{
};

class Allocator
{
public:
    static void* Alloc(int size);
    static int Free(void* memptr);

private:
    int sAllocCount;
};

class FileIO
{
public:
    static FileHandle* Open(const char* filename);
    static FileHandle* WOpen(const char* filename);
    static int Close(FileHandle* g);
    static int Read(FileHandle* g, void* buf, int size);
    static int Write(FileHandle* g, const void* buf, int size);
    static int Seek(FileHandle* g, __int64 offset);
    static __int64 Length(FileHandle* g);
    static __int64 Tell(FileHandle* g);

private:
    struct FileIOHandle
    {
        FILE* handle;
        int offset;
        int len;
        int cachestart;
        char* bufptr;
        char* bufend;
        int write;
        int dirty;
        int gopenmethod;
        char filename[1024];
        char tempname[1024];
        char buf[8192];
    };

    static void MakeTempName(char* d, const char* s, const char* ext);
    static int IsReadOnly(const char* filename);
    static int IsDirectory(const char* filename);
    static __int64 WriteFlush(FileIOHandle* h);
    static int sOpenMethod;
};

#endif
