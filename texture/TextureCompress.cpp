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

#include "TextureCompress.h"
#include <stdlib.h>
#include <string.h>
#include <dxtlib\dxtlib.h>
#include <d3dx9.h>
#include <xgraphics.h>

TextureBuffer::TextureBuffer(size_t capacity) : Size(0)
{
	Reserve(capacity);
}

TextureBuffer::~TextureBuffer()
{
	if (Data)
	{
		free(Data);
		Data = nullptr;
	}

	Size = 0;
	Capacity = 0;
}

void TextureBuffer::Reserve(size_t count)
{
	Capacity = count;
	Data = malloc(Capacity);
}

void TextureBuffer::Reset()
{
	Size = 0;
}

void TextureBuffer::Zero()
{
	Reset();
	memset(Data, 0, Capacity);
}

void TextureBuffer::Append(const void* data, size_t count)
{
	if (Size + count > Capacity)
	{
		while (Size + count > Capacity)
		{
			Capacity *= 2;
		}

		Data = realloc(Data, Capacity);
	}

	memcpy(((char*)Data) + Size, data, count);
	Size += count;
}

TextureBuffer* buffer;
IDirect3D9* d3d;
IDirect3DDevice9* d3ddevice;

void D3DInit()
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS pp = {};
	pp.BackBufferWidth = 640;
	pp.BackBufferHeight = 480;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.MultiSampleType = D3DMULTISAMPLE_NONE;
	pp.MultiSampleQuality = 0;
	pp.BackBufferCount = 1;
	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = D3DFMT_D24S8;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	d3d->CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &d3ddevice);
	buffer = new TextureBuffer(2097152);
}

void D3DShutdown()
{
	d3ddevice->Release();
	d3d->Release();
	delete buffer;
}

bool IsPowerOfTwo(int value)
{
	return value ? ((value & (~value + 1)) == value) : false;
}

bool IsValidTexture(RGBAImage &image)
{
	if (image.size() <= 0)
	{
		return false;
	}

	if (!IsPowerOfTwo(image.width()) || !IsPowerOfTwo(image.height()))
	{
		return false;
	}

	return true;
}

bool HasAlphaChannel(D3DFORMAT format)
{
	switch (format)
	{
	case D3DFMT_LIN_DXT2:
	case D3DFMT_LIN_DXT3A:
	case D3DFMT_LIN_DXT3A_1111:
	case D3DFMT_LIN_DXT5:
	case D3DFMT_LIN_DXT5A:
	case D3DFMT_LIN_A1R5G5B5:
	case D3DFMT_LIN_A4R4G4B4:
	case D3DFMT_LIN_A8R8G8B8:
	case D3DFMT_LIN_A8B8G8R8:
	case D3DFMT_LIN_A2R10G10B10:
	case D3DFMT_LIN_A2B10G10R10:
	case D3DFMT_LIN_A2W10V10U10:
	case D3DFMT_LIN_A16B16G16R16:
	case D3DFMT_LIN_A16B16G16R16F:
	case D3DFMT_LIN_A16B16G16R16F_EXPAND:
	case D3DFMT_LIN_A32B32G32R32:
	case D3DFMT_LIN_A32B32G32R32F:
	{
		return true;
	}
	}

	return false;
}

void LoadImage(RGBAImage& image, bool& alpha, char const* const path)
{
	LPDIRECT3DTEXTURE9 texture;

	if (FAILED(D3DXCreateTextureFromFile(d3ddevice, path, &texture)))
	{
		// error handling not implemented
	}

	XGTEXTURE_DESC desc;
	XGGetTextureDesc(texture, 0, &desc);
	alpha = HasAlphaChannel((D3DFORMAT)MAKELINFMT(desc.Format));
	D3DLOCKED_RECT rect;
	texture->LockRect(0, &rect, nullptr, 0);
	image.resize(desc.Width, desc.Height);

	if (FAILED(XGCopySurface(image.pixels(), sizeof(rgba_t) * desc.Width, desc.Width, desc.Height, (D3DFORMAT)MAKELEFMT(D3DFMT_LIN_A8B8G8R8), nullptr, rect.pBits, rect.Pitch, (D3DFORMAT)MAKELEFMT(desc.Format), nullptr, XGCOMPRESS_ALPHADIVIDE | XGCOMPRESS_NO_DITHERING, 0.0f)))
	{
		// error handling not implemented
	}

	texture->UnlockRect(0);

	if (texture->Release() != 0)
	{
		// error handling not implemented
	}
}

bool Has1BitAlphaChannel(RGBAImage& image)
{
	size_t size = image.size();

	for (size_t s = 0; s < size; s++)
	{
		rgba_t& pixel = image.pixel_ref(s);

		if (pixel.a != 0 && pixel.a != 255)
		{
			return false;
		}
	}

	return true;
}

bool HasConstantAlphaChannel(RGBAImage& image)
{
	size_t size = image.size();
	rgba_t& first = image.pixel_ref(0);

	for (size_t s = 0; s < size; s++)
	{
		rgba_t& pixel = image.pixel_ref(s);

		if (pixel.a != first.a)
		{
			return false;
		}
	}

	return true;
}

void ZeroAlphaChannel(RGBAImage& image)
{
	for (size_t x = 0; x < image.width(); x++)
	{
		for (size_t y = 0; y < image.height(); y++)
		{
			image.pixelsXY_ref(x, y).a = 0;
		}
	}
}

void OneAlphaChannel(RGBAImage& image)
{
	for (size_t x = 0; x < image.width(); x++)
	{
		for (size_t y = 0; y < image.height(); y++)
		{
			image.pixelsXY_ref(x, y).a = 255;
		}
	}
}

rgba_t& GetPixel(RGBAImage& image, double u, double v)
{
	if (u < 0)
	{
		u = (u - ceil(u)) + 1.0f;
	}
	else
	{
		u = u - floor(u);
	}

	if (v < 0)
	{
		v = (v - ceil(v)) + 1.0f;
	}
	else
	{
		v = v - floor(v);
	}

	int x = (int)(u * image.width());
	int y = (int)(v * image.height());
	return image.pixelsXY_ref(x, y);
}

void CreateVolumeRGBAImage(RGBAMipMappedVolumeMap& dst, RGBAImage& src)
{
	if (src.width() % src.height() != 0)
	{
		// error handling not implemented
	}

	size_t h = src.height();
	size_t w = h;
	size_t d = src.width() / h;
	dst.resize(w, h, d, 1);
	RGBAImageArray& vol = dst[0];

	for (size_t i = 0; i < d; i++)
	{
		size_t offset = w * i;
		RGBAImage& layer = vol[i];
		
		for (size_t x = 0; x < w; x++)
		{
			for (size_t y = 0; y < h; y++)
			{
				layer.pixelsXY_ref(x, y) = src.pixelsXY_ref(offset + x, y);
			}
		}
	}
}

std::string GuessFormat(bool alpha, RGBAImage& image, std::string format)
{
	if (format == "D3DFMT_DXT1" && alpha)
	{
		if (HasConstantAlphaChannel(image))
		{
			if (image.pixel_ref(0).a == 255)
			{
				return format;
			}

			return "D3DFMT_DXT3";
		}

		if (Has1BitAlphaChannel(image))
		{
			return "D3DFMT_DXT3";
		}

		return "D3DFMT_DXT5";
	}

	return format;
}

nvCompressionOptions GetNVCompressionOptions(std::string format, bool mip)
{
	nvCompressionOptions options;

	if (format == "D3DFMT_A8R8G8B8")
	{
		options.textureFormat = k8888;
	}
	else if (format == "D3DFMT_DXT5")
	{
		options.textureFormat = kDXT5;
	}
	else if (format == "D3DFMT_DXT3")
	{
		options.textureFormat = kDXT3;
	}
	else if (format == "D3DFMT_DXT1A")
	{
		options.textureFormat = kDXT1a;
	}
	else if (format == "D3DFMT_X8R8G8B8")
	{
		options.textureFormat = kX888;
	}
	else if (format == "D3DFMT_R5G6B5")
	{
		options.textureFormat = k565;
	}
	else
	{
		options.textureFormat = kDXT1;
	}

	if (mip)
	{
		options.GenerateMIPMaps(0);
	}
	else
	{
		options.DoNotGenerateMIPMaps();
	}

	return options;
}

NV_ERROR_CODE callback(const void* buffer, size_t count, const MIPMapData* mipMapData, void* userData)
{
	TextureBuffer* t = static_cast<TextureBuffer*>(userData);
	t->Append(buffer, count);
	return NV_OK;
}

TextureBuffer* CompressTexture(RGBAImage& image, nvCompressionOptions& options)
{
	buffer->Reset();
	options.user_data = buffer;
	options.textureType = kTextureTypeTexture2D;
	NV_ERROR_CODE result = nvDDS::nvDXTcompress(image, &options, callback);

	if (result != NV_OK)
	{
		// error handling not implemented
	}

	if (buffer->Size == 0)
	{
		// error handling not implemented
	}

	return buffer;
}

TextureBuffer* CompressVolumeTexture(RGBAImage& image, nvCompressionOptions& options)
{
	buffer->Reset();
	options.user_data = buffer;
	options.textureType = kTextureTypeVolumeMap;
	RGBAMipMappedVolumeMap volume;
	CreateVolumeRGBAImage(volume, image);
	NV_ERROR_CODE result = nvDDS::nvDXTcompress(volume, &options, callback);
	
	if (result != NV_OK)
	{
		// error handling not implemented
	}

	if (buffer->Size == 0)
	{
		// error handling not implemented
	}

	return buffer;
}

TextureBuffer* Compress(std::string path, std::string type, std::string format, bool mip)
{
	bool alpha;
	RGBAImage image;
	LoadImage(image, alpha, path.c_str());

	if (IsValidTexture(image))
	{
		format = GuessFormat(alpha, image, format);
		nvCompressionOptions options = GetNVCompressionOptions(format, mip);
		
		if (type == "VolumeTexture")
		{
			return CompressVolumeTexture(image, options);
		}
		else if (type == "CubeTexture")
		{
			// error handling not implemented
		}
		else if (type == "StandardTexture")
		{
			return CompressTexture(image, options);
		}
	}

	return nullptr;
}
