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

#include "base.h"
#include "encoders\encpcm16big.h"
#include "endian.h"

EncoderDesc Pcm16BigEnc::sEncoderDesc = { GUID, CreateInstance, 0, 0, 0, true };

EncoderDesc* Pcm16BigEnc::GetEncoderDesc()
{
    return &sEncoderDesc;
}

Encoder* Pcm16BigEnc::CreateInstance(int numChannels, int sampleRate, System* pSystem)
{
    Pcm16BigEnc* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        goto abort;
    }

    pThis->mAverageDataRate = 2 * (float)(sampleRate * numChannels);
    pThis->mVbrQuality = 0.0f;
    pThis->mCbrRate = 4000;
    return static_cast<Encoder*>(pThis);

abort:
    return 0;
}

int Pcm16BigEnc::Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    short* pShortDst = reinterpret_cast<short*>(pDst);
    int totalframes = GetChannels() * numSamplesIn;

    for (int frame = 0; frame < totalframes; frame++)
    {
        short temp = static_cast<short>(*pSrc++ * 32767.0f);
        ENDIAN::PutB(*pShortDst++, temp);
    }

    *pBytesEncoded = 2 * totalframes;

    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    return numSamplesIn;
}

int Pcm16BigEnc::Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    *pBytesEncoded = 0;

    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    return 0;
}
