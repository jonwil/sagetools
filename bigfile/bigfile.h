/*
**	sagetools
**	Copyright 2026 Jonathan Wilson
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BIGFILE_H
#define BIGFILE_H

#include <stdio.h>
#include <vector>
#include <string>

#if defined(_MSC_VER)
#pragma once
#endif

struct BigHeader
{
	unsigned int ID;
	unsigned int length;
	unsigned int filecount;
	unsigned int headersize;
};

struct BigEntry
{
	unsigned int offset;
	unsigned int size;
	unsigned int unpackedsize;
	bool compressed;
	const char* name;
};

class BigReader
{
public:
	BigReader(const char* filename);
	~BigReader();
	int Get_File_Count();
	BigEntry Get_File_Entry(int index);
	void Get_File_Data(int index, char* buf);
private:
	BigHeader header;
	char* headerbuf;
	int version;
	int flags;
	FILE* handle;
	std::vector<BigEntry> entries;
};

struct BigWriteEntry
{
	std::string srcfilename;
	std::string destfilename;
	bool compressed;
};

class BigWriter
{
public:
	BigWriter(const char* outfilename);
	~BigWriter();
	void Add_File(const char* srcfilename, const char *destfilename, bool compressed);
	void Set_Compress(bool set) { compress = set; }
private:
	FILE* handle;
	std::vector<BigWriteEntry> filenames;
	int headersize;
	bool compress;
};

#endif
