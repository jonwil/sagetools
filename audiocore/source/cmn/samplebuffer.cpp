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

#include "samplebuffer.h"

unsigned int SampleBuffer::GetSize(unsigned int numChannels, unsigned int maxLocks, unsigned int maxSamples, unsigned int* pAlignment, System* pSystem)
{
    *pAlignment = 16;
    unsigned int memRequired = 20;
    return memRequired;
}

SampleBuffer* SampleBuffer::CreateInstance(unsigned int numChannels, unsigned int maxLocks, unsigned int maxSamples, void* pMem, void* pStorage, System* pSystem)
{
    SampleBuffer* pThis = static_cast<SampleBuffer*>(pMem);
    pThis->mpSystem = pSystem;
    pThis->mNumSamples = 0;
    pThis->mMaxSamples = maxSamples;
    pThis->mNumChannels = numChannels;
    pThis->mpStorage = static_cast<float*>(pStorage);
    return pThis;
}
