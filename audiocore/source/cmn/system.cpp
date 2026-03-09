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

#include "system.h"
#include "decoderregistry.h"
#include "encoderregistry.h"
#include "ibase.h"
#include <windows.h>

DetectCPU staticDetectCPU;
System* System::spInstance;

System* System::CreateInstance()
{
    System* pThis = static_cast<System*>(_aligned_malloc(sizeof(System), 16));

    if (!pThis)
    {
        goto abort;
    }

    pThis = new(pThis) System();
    spInstance = pThis;
    pThis->mpDecoderRegistry = 0;
    pThis->mpEncoderRegistry = 0;
    return pThis;

abort:
    if (pThis)
    {
        pThis->~System();
        _aligned_free(pThis);
    }

    return 0;
}

void System::Release()
{
    Lock();

    if (mpDecoderRegistry)
    {
        mpDecoderRegistry->Release();
    }

    if (mpEncoderRegistry)
    {
        mpEncoderRegistry->Release();
    }

    Unlock();
    this->~System();
    _aligned_free(this);
    spInstance = 0;
}

void System::Lock()
{
    mpMutex.lock();
}

void System::Unlock()
{
    mpMutex.unlock();
}

void* System::Alloc(unsigned int size, unsigned int alignment)
{
    void* pMem = _aligned_malloc(size, alignment);
    return pMem;
}

void System::Free(void* pMem)
{
    _aligned_free(pMem);
}

EncoderRegistry* System::GetEncoderRegistry()
{
    if (!mpEncoderRegistry)
    {
        mpEncoderRegistry = EncoderRegistry::CreateInstance();
    }

    return mpEncoderRegistry;
}

DecoderRegistry* System::GetDecoderRegistry()
{
    if (!mpDecoderRegistry)
    {
        mpDecoderRegistry = DecoderRegistry::CreateInstance();
    }

    return mpDecoderRegistry;
}
