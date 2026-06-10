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

#ifndef TEXTURECOMPRESS_H
#define TEXTURECOMPRESS_H

#include <string>

struct TextureBuffer
{
	void* Data;
	size_t Size;
	size_t Capacity;

	TextureBuffer(size_t capacity);
	~TextureBuffer();
	void Reserve(size_t count);
	void Reset();
	void Zero();
	void Append(const void* data, size_t count);
};

void D3DInit();
void D3DShutdown();
TextureBuffer* Compress(std::string path, std::string type, std::string format, bool mip);

#endif
