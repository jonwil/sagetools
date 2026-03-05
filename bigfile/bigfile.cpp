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

#include "bigfile.h"
#include "codex.h"
#include "refcodex.h"
#include <filesystem>

BigReader::BigReader(const char* filename)
{
	handle = fopen(filename, "rb");
	char buf[16];
	fread(buf, 1, 16, handle);
	header.ID = ggetm(buf, 4);
	header.length = ggetm(buf + 4, 4);
	header.filecount = ggetm(buf + 8, 4);
	header.headersize = ggetm(buf + 12, 4);
	fseek(handle, 0, SEEK_SET);
	headerbuf = new char[header.headersize];
	fread(headerbuf, 1, header.headersize, handle);
	const char* headerptr = headerbuf + 16;
	const char* debugptr = headerbuf + header.headersize - 8;
	
	if (isalpha(debugptr[0]) && isdigit(debugptr[1]) && isdigit(debugptr[2]) && isdigit(debugptr[3]))
	{
		version = (debugptr[1] - '0') * 100 + (debugptr[2] - '0') * 10 + (debugptr[3] - '0');
		flags = ggetm(debugptr + 4, 4);
	}

	for (unsigned int i = 0; i < header.filecount; i++)
	{
		BigEntry entry;
		entry.offset = ggetm(headerptr, 4);
		entry.size = ggetm(headerptr + 4, 4);
		entry.name = headerptr + 8;
		entry.compressed = false;
		entry.unpackedsize = entry.size;

		if (entry.size > 12)
		{
			fseek(handle, entry.offset, SEEK_SET);
			char fbuf[12];
			fread(fbuf, 1, 12, handle);

			if (REF_is(fbuf))
			{
				entry.unpackedsize = REF_size(fbuf);
				entry.compressed = true;
			}
		}

		headerptr += 8 + strlen(entry.name) + 1;
		entries.push_back(entry);
	}
}

BigReader::~BigReader()
{
	fclose(handle);
	delete[] headerbuf;
}

int BigReader::Get_File_Count()
{
	return header.filecount;
}

BigEntry BigReader::Get_File_Entry(int index)
{
	return entries[index];
}

void BigReader::Get_File_Data(int index, char *buf)
{
	BigEntry entry = entries[index];

	if (entry.compressed)
	{
		char* cbuf = new char[entry.size];
		fseek(handle, entry.offset, SEEK_SET);
		fread(cbuf, 1, entry.size, handle);
		REF_decode(buf, cbuf);
		delete[] cbuf;
	}
	else
	{
		fseek(handle, entry.offset, SEEK_SET);
		fread(buf, 1, entry.size, handle);
	}
}

BigWriter::BigWriter(const char* outfilename)
{
	handle = fopen(outfilename, "w+b");
	headersize = 24;
}

static void pad(FILE* f)
{
	int pos = ftell(f);
	int pad = 64 - (int)(pos % 64);

	if (pad == 64)
	{
		pad = 0;
	}

	if (pad)
	{
		char c = 0;

		for (int i = 0; i < pad; i++)
		{
			fwrite(&c, 1, 1, f);
		}
	}
}

BigWriter::~BigWriter()
{
	char* headerbuf = new char[headersize];
	char* headerptr = headerbuf + 16;
	memset(headerbuf, 0, headersize);
	fwrite(headerbuf, 1, headersize, handle);
	fflush(handle);
	int offset = 0;

	for (BigWriteEntry &entry : filenames)
	{
		int filesize;
		void* buf = 0;
		FILE* in = fopen(entry.srcfilename.c_str(), "rb");
		fseek(in, 0, SEEK_END);
		filesize = ftell(in);
		fseek(in, 0, SEEK_SET);
		buf = new char[filesize];
		fread(buf, 1, filesize, in);
		fseek(in, 0, SEEK_SET);

		if (entry.compressed && filesize > 0 && filesize < 0x40000000)
		{
			int compsize;
			int packsize = (int)(filesize * 1.2) + 1024;
			void* packbuf = new char[packsize];
			compsize = REF_encode(packbuf, buf, filesize);

			if (compsize >= filesize - 1024 || compsize >= packsize)
			{
				delete[] packbuf;
			}
			else
			{
				delete[] buf;
				buf = packbuf;
				filesize = compsize;
			}
		}

		pad(handle);
		offset = ftell(handle);
		fwrite(buf, 1, filesize, handle);
		delete[] buf;
		fclose(in);
		gputm(headerptr, offset, 4);
		gputm(headerptr + 4, filesize, 4);
		strcpy(headerptr + 8, entry.destfilename.c_str());
		headerptr += 9;
		headerptr += strlen(entry.destfilename.c_str());
	}

	fseek(handle, 0, SEEK_END);
	offset = ftell(handle);
	gputm(headerbuf, 'BIG4', 4);
	gputi(headerbuf + 4, offset, 4);
	int count = filenames.size();
	gputm(headerbuf + 8, count, 4);
	gputm(headerbuf + 12, headersize, 4);
	int flags = 0x15050000;
	
	if (compress)
	{
		flags |= 1;
	}

	gputm(headerbuf + headersize - 8, 'L280', 4);
	gputm(headerbuf + headersize - 4, flags, 4);
	fseek(handle, 0, SEEK_SET);
	fwrite(headerbuf, 1, headersize, handle);
	delete[] headerbuf;
	fclose(handle);
}

void BigWriter::Add_File(const char* srcfilename, const char* destfilename, bool compressed)
{
	BigWriteEntry entry;
	entry.srcfilename = srcfilename;
	entry.destfilename = destfilename;
	entry.compressed = compressed;
	std::transform(entry.destfilename.begin(), entry.destfilename.end(), entry.destfilename.begin(), [](unsigned char c) { return std::tolower(c); });
	filenames.push_back(entry);
	headersize += 9;
	headersize += strlen(destfilename);
}
