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

#include "coda\include\coda.h"

void* CSign24IntDecS16::operator new(size_t size)
{
    return CODANew(size);
}

void CSign24IntDecS16::operator delete(void* ptr)
{
    CODADelete(ptr);
}

CSign24IntDecS16::CSign24IntDecS16()
{
    mRemainingSamples = 0;
    mSampleDataSize = 0;
    mChannels = 1;
}

int CSign24IntDecS16::Feed(void* pSampleData, int sampleDataSize, int numSamples)
{
    if (!pSampleData)
    {
        return -1;
    }

    if (mRemainingSamples)
    {
        return -1;
    }
    else
    {
        mRemainingSamples = numSamples;
    }

    mSampleDataSize = sampleDataSize;
    mpSrc = (unsigned char*)pSampleData;
    return 0;
}

int CSign24IntDecS16::Decode(short* pDstBuf[], int numSamples)
{
    if (!mRemainingSamples)
    {
        return 0;
    }

    int maxsamples = numSamples < mRemainingSamples ? numSamples : mRemainingSamples;
    int i;
    short chan = 0;

    for (i = 0; i < maxsamples; i++)
    {
        for (chan = 0; chan < mChannels; chan++)
        {
            mpDst = &pDstBuf[chan][i];
            *mpDst = (mpSrc[2] << 8) | mpSrc[1];
            mpSrc += 3;
        }
    }

    mRemainingSamples -= maxsamples;
    return maxsamples;
}

int CSign24IntDecS16::GetState()
{
    return mChannels;
}

void CSign24IntDecS16::SetState(short* pchannel)
{
    mChannels = *pchannel;
}
