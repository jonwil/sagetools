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

#include <string.h>
#include <filesystem>
#include "bigfile.h"

static int strcmplogical(const char* s0, const char* s1)
{
	int diff = 0;

	while ((*s0 || *s1) && !diff)
	{
		diff = (unsigned char)*s0 - (unsigned char)*s1;

		if (diff && (isdigit(*s0) || isdigit(*s1)))
		{
			int n0 = -1;

			if (isdigit(*s0))
			{
				char digits[100];
				char* d = digits;

				while (isdigit(*s0))
				{
					if (d < &digits[99])
					{
						*d++ = *s0;
					}
					
					s0++;
				}

				*d = '\0';
				n0 = atoi(digits);
			}

			int n1 = -1;

			if (isdigit(*s1))
			{
				char digits[100];
				char* d = digits;

				while (isdigit(*s1))
				{
					if (d < &digits[99])
					{
						*d++ = *s1;
					}

					s1++;
				}

				*d = '\0';
				n1 = atoi(digits);
			}

			diff = n0 - n1;
		}
		else
		{
			s0++;
			s1++;
		}
	}

	return diff;
}

char* strtrim(char* buffer)
{
	if (buffer)
	{
		char* source = buffer;

		while ((*source != 0) && ((unsigned char)*source <= 32))
		{
			source++;
		}

		if (source != buffer)
		{
			strcpy(buffer, source);
		}

		for (int index = strlen(buffer) - 1; index >= 0; index--)
		{
			if ((*source != 0) && ((unsigned char)buffer[index] <= 32))
			{
				buffer[index] = '\0';
			}
			else
			{
				break;
			}
		}
	}

	return buffer;
}

char* strripquotes(char* buffer)
{
	if (buffer)
	{
		char* source = buffer;

		while ((*source != 0) && ((unsigned char)*source == '\"'))
		{
			source++;
		}

		if (source != buffer)
		{
			strcpy(buffer, source);
		}

		for (int index = strlen(buffer) - 1; index >= 0; index--)
		{
			if ((*source != 0) && ((unsigned char)buffer[index] == '\"'))
			{
				buffer[index] = '\0';
			}
			else
			{
				break;
			}
		}
	}

	return buffer;
}

int main(int argc, char* argv[])
{
	const char* big = nullptr;
	const char* path = nullptr;
	bool create = false;
	bool compress = false;
	bool list = false;

	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			char* arg = &argv[i][1];
			bool done = false;

			if (!_stricmp(arg, "big"))
			{
				big = argv[i + 1];
			}
			else if (!_stricmp(arg, "path"))
			{
				path = argv[i + 1];
				list = false;
			}
			else if (!_stricmp(arg, "create"))
			{
				create = true;
			}
			else if (!_stricmp(arg, "compress"))
			{
				compress = true;
			}
			else if (!_stricmp(arg, "list"))
			{
				path = argv[i + 1];
				list = true;
			}
		}
	}

	if (big && path)
	{
		if (!create)
		{
			BigReader reader(big);
			std::filesystem::path oldpath = std::filesystem::current_path();
			std::filesystem::current_path(path);
			
			for (int i = 0; i < reader.Get_File_Count(); i++)
			{
				BigEntry entry = reader.Get_File_Entry(i);
				std::filesystem::path filepath(entry.name);
				std::filesystem::path filename = filepath.filename();
				filepath.remove_filename();
				std::filesystem::create_directories(filepath);
				filepath += filename;
				FILE* out = _wfopen(filepath.c_str(), L"wb");
				char* buf = new char[entry.unpackedsize];
				reader.Get_File_Data(i, buf);
				fwrite(buf, 1, entry.unpackedsize, out);
				fclose(out);
				delete[] buf;
			}

			std::filesystem::current_path(oldpath);
		}
		else
		{
			BigWriter writer(big);

			if (!list)
			{
				std::vector<std::string> files;

				for (const std::filesystem::directory_entry& dir_entry : std::filesystem::recursive_directory_iterator(path))
				{
					if (!dir_entry.is_directory())
					{
						std::string file = dir_entry.path().string();
						files.push_back(file);
					}
				}

				std::sort(files.begin(), files.end(), [](std::string a, std::string b) {return strcmplogical(a.c_str(), b.c_str()) < 0; });

				for (std::string& file : files)
				{
					if (std::filesystem::path(file).extension().string() == ".asset")
					{
						continue;
					}

					std::string src;

					std::string::size_type pos = file.find(path);

					if (pos == std::string::npos)
					{
						src = path;
						src += "\\";
						src += file;
					}
					else
					{
						src = file;
						file.erase(pos, strlen(path) + 1);
					}
					
					if (std::filesystem::path(file).extension().string() == ".cdata" || std::filesystem::path(file).extension().string() == ".vp6" || std::filesystem::path(file).extension().string() == ".snd" || std::filesystem::path(file).extension().string() == ".imp")
					{
						writer.Add_File(src.c_str(), file.c_str(), false);
					}
					else
					{
						writer.Add_File(src.c_str(), file.c_str(), compress);
					}
				}

				writer.Set_Compress(compress);
			}
			else
			{
				FILE* resp = fopen(path, "rt");
				char buf[1024];
				std::vector<std::pair<std::string, std::string>> options;
				bool comp = false;

				while (fgets(buf, sizeof(buf), resp))
				{
					strtrim(buf);
					std::string str = buf;

					if (str != "-packthresh1024")
					{
						if (str == "-pack0")
						{
							comp = false;
							options.push_back(std::make_pair("-pack0", "-pack0"));
						}
						else if (str == "-pack1")
						{
							comp = true;
							options.push_back(std::make_pair("-pack1", "-pack1"));
						}
						else
						{
							char* src = strtok(buf, "=");
							char* dest = strtok(0, "=");
							strripquotes(src);
							strripquotes(dest);
							options.push_back(std::make_pair(src, dest));
						}
					}
				}

				fclose(resp);

				for (std::pair<std::string, std::string>& str : options)
				{
					if (str.first == "-pack0")
					{
						comp = false;
					}
					else if (str.first == "-pack1")
					{
						comp = true;
					}
					else
					{
						writer.Add_File(str.first.c_str(), str.second.c_str(), comp);
					}
				}

				writer.Set_Compress(comp);
			}
		}
	}

	return 0;
}
