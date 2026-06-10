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
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <dxtlib\dxtlib.h>

namespace std
{
    class xxception
    {
    public:
        xxception();
        xxception(const char* const&);
        xxception(const xxception&);
        virtual  ~xxception();
        virtual const char* what() const;
    private:
        const char* _m_what;
        int _m_doFree;
    };

    xxception::xxception()
    {
        _m_what = NULL;
        _m_doFree = 0;
    }

    xxception::xxception(const char* const& what)
    {
        if (what)
        {
            const size_t _Buf_size = strlen(what) + 1;
            _m_what = static_cast<const char*>(malloc(_Buf_size));

            if (_m_what)
            {
                strcpy_s(const_cast<char*>(_m_what), _Buf_size, what);
            }
        }
        else
        {
            _m_what = NULL;
        }

        _m_doFree = true;
    }

    xxception::xxception(const xxception& that)
    {
        _m_doFree = that._m_doFree;

        if (_m_doFree)
        {
            if (that._m_what)
            {
                const size_t _Buf_size = strlen(that._m_what) + 1;
                _m_what = static_cast<const char*>(malloc(_Buf_size));

                if (_m_what)
                {
                    strcpy_s(const_cast<char*>(_m_what), _Buf_size, that._m_what);
                }
            }
            else
            {
                _m_what = NULL;
            }

        }
        else
        {
            _m_what = that._m_what;
        }
    }

    xxception::~xxception()
    {
        if (_m_doFree)
        {
            free(const_cast<char*>(_m_what));
        }
    }

    const char* xxception::what() const
    {
        if (_m_what != NULL)
        {
            return _m_what;
        }
        else
        {
            return "Unknown exception";
        }
    }

    struct _Xontainer_base
    {
    };

    class  _Xtring_base : public _Xontainer_base
    {
    public:
        static void __cdecl _Xlen();

        static void __cdecl _Xran();
    };

    void __cdecl _Xtring_base::_Xlen()
    {
        throw length_error("string too long");
    }

    void __cdecl _Xtring_base::_Xran()
    {
        throw out_of_range("invalid string position");
    }
}

void* address;
void* address2;
void* hook;
struct DDS_HEADER;

class nvDXTCompression
{
public:
    NV_ERROR_CODE DXTCencode(DDS_HEADER*, DDS_HEADER*, unsigned char*, nvTextureFormats, int);
};

void __declspec(naked) hookfunc()
{
    _asm
    {
        mov [esp - 3850h], 34h
        push ebp
        mov ebp, esp
        and esp, 0FFFFFFF8h
        mov eax, address2
        jmp eax
    }
}

static const char jumpOp = '\xE9';

void WriteMemory(void* lpBaseAddress, const void* lpBuffer, size_t nSize)
{
    DWORD dwOldProtect;
    DWORD dwOldProtect2;
    HANDLE hProcess = GetCurrentProcess();
    VirtualProtectEx(hProcess, lpBaseAddress, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
    WriteProcessMemory(hProcess, lpBaseAddress, lpBuffer, nSize, NULL);
    VirtualProtectEx(hProcess, lpBaseAddress, nSize, dwOldProtect, &dwOldProtect2);
}

void WriteJump(void* location, void* function)
{
    if (!location)
        return;

    char* offset = (char*)((char*)function - (char*)location - 5);
    WriteMemory(location, &jumpOp, 1);
    WriteMemory((char*)location + 1, &offset, 4);
}

void installhookfunc()
{
    __asm
    {
        mov eax, nvDXTCompression::DXTCencode
        mov dl, 0E9h
        cmp [eax], dl
        jne loc
        inc eax
        add eax, [eax]
        add eax, 4
    loc:
        mov address, eax
        add eax, 6
        mov address2, eax
        mov eax, hookfunc
        mov hook, eax
    }

    WriteJump(address, hook);
}
