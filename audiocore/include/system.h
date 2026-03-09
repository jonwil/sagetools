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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "base.h"
#include "private\linklist.h"
#include <mutex>

class Decoder;
class DecoderRegistry;
class Encoder;
class EncoderRegistry;

class System
{
public:
    static System* CreateInstance();
    static System* GetInstance() { return spInstance; }
    void Release();
    DecoderRegistry* GetDecoderRegistry();
    EncoderRegistry* GetEncoderRegistry();
    void Lock();
    void Unlock();
    void* Alloc(unsigned int size, unsigned int alignment = 16);
    void Free(void* pMem);

    template <class TClass> void New2(TClass** ppThis, unsigned int size = 0, unsigned int alignment = 16)
    {
        size = (size ? size : sizeof(TClass));
        *ppThis = static_cast<TClass*>(Alloc(size, alignment));

        if (!*ppThis)
        {
            return;
        }

        *ppThis = new (*ppThis) TClass();
    }

    template<class TClass> void Delete(TClass pClass)
    {
        pClass.~TClass();
        Free(pClass);
    }

private:
    System(const System&) {}
    System& operator=(const System&) { return *this; }
    System() {};
    ~System() {};

    static System* spInstance;
    DecoderRegistry* mpDecoderRegistry;
    EncoderRegistry* mpEncoderRegistry;
    std::mutex mpMutex;
};

#endif
