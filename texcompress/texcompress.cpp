#include <stdio.h>
#include <string>
#include <filesystem>

#include "TextureCompress.h"
void installhookfunc();

struct TextureFileData
{
    void* m_Data;
    size_t m_DataSize;
};

int main(int argc, char** argv)
{
    installhookfunc();
    D3DInit();
    std::string fname;
    std::string outfile;
    std::string outformat = "DXT1";
    std::string format;
    std::string type = "StandardTexture";
    bool mipmaps = true;

    for (int i = 0; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            char* arg = &argv[i][1];

            if (!_stricmp(arg, "in"))
            {
                fname = argv[i + 1];
            }
            else if (!_stricmp(arg, "out"))
            {
                outfile = argv[i + 1];
            }
            else if (!_stricmp(arg, "type"))
            {
                type = argv[i + 1];
            }
            else if (!_stricmp(arg, "generatemips"))
            {
                mipmaps = argv[i + 1][0] == 't' || argv[i + 1][0] == 'T';
            }
            else if (!_stricmp(arg, "format"))
            {
                outformat = argv[i + 1];
            }
        }
    }

    if (fname.size() && outfile.size() && outformat.size() && type.size())
    {
        bool dds = fname.ends_with(".dds");

        if (!std::filesystem::exists(fname))
        {
            // error handling not implemented
        }

        TextureFileData texture;
        texture.m_DataSize = 0;
        texture.m_Data = 0;

        if (outformat == "A8R8G8B8")
        {
            format = "D3DFMT_A8R8G8B8";
        }
        else if (outformat == "DXT1")
        {
            format = "D3DFMT_DXT1";
        }
        else if (outformat == "DXT1A")
        {
            format = "D3DFMT_DXT1A";
        }
        else if (outformat == "DXT3")
        {
            format = "D3DFMT_DXT3";
        }
        else if (outformat == "DXT5")
        {
            format = "D3DFMT_DXT5";
        }
        else if (outformat == "X8R8G8B8")
        {
            format = "D3DFMT_X8R8G8B8";
        }
        else if (outformat == "R5G6B5")
        {
            format = "D3DFMT_R5G6B5";
        }

        if (dds)
        {
            FILE* f = fopen(fname.c_str(), "rb");
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);

            if (!size)
            {
                // error handling not implemented
            }

            texture.m_DataSize = (size_t)size;
            texture.m_Data = malloc(texture.m_DataSize);
            fread(texture.m_Data, 1, size, f);
            fclose(f);
        }
        else
        {
            TextureBuffer* buffer = Compress(fname, type, format, mipmaps);
            texture.m_Data = malloc(buffer->Size);
            texture.m_DataSize = buffer->Size;
            memcpy(texture.m_Data, buffer->Data, buffer->Size);
        }

        FILE* out = fopen(outfile.c_str(), "wb");
        fwrite(texture.m_Data, 1, texture.m_DataSize, out);
        fclose(out);
        free(texture.m_Data);
    }
    
    D3DShutdown();
    return 0;
}
