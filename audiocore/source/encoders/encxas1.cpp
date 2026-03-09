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
#include "ibase.h"
#include "encoders\encxas1.h"
#include <string.h>

EncoderDesc Xas1Enc::sEncoderDesc = { GUID, CreateInstance, 0, 0, 0, true };

EncoderDesc* Xas1Enc::GetEncoderDesc()
{
    return &sEncoderDesc;
}

const float Xas1Enc::sFilterPairs[4][2] =
{
    { 0.0f,         0.0f       },
    { -0.9375f,     0.0f       },
    { -1.796875f,   0.8125f    },
    { -1.53125f,    0.859375f  }
};

Encoder* Xas1Enc::CreateInstance(int numChannels, int sampleRate,
    System* pSystem)
{
    Xas1Enc* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        goto abort;
    }

    float ratio;
    ratio = static_cast<float>(XAS_BLOCKBYTES) / static_cast<float>(XAS_BLOCKSAMPLES);
    pThis->mAverageDataRate = ratio * static_cast<float>(sampleRate * numChannels);
    pThis->mVbrQuality = 0.0f;
    pThis->mCbrRate = 4000;
    pThis->mResidualSamples = 0;
    return static_cast<Encoder*>(pThis);

abort:
    return 0;
}

void Xas1Enc::CustomInterleaveXAS(unsigned char* pSrcBlock, unsigned char* pDstBlock)
{
    for (int i = 0; i < XAS_MINIBLOCKGROUP; i++)
    {
        memcpy(pDstBlock + (i * 4), pSrcBlock + (i * XAS_MINIBLOCKBYTES), 4);
    }

    unsigned char* pSrc = pSrcBlock + 4;
    unsigned char* pDst = pDstBlock + (XAS_MINIBLOCKGROUP * 4);

    for (int i = 0; i < XAS_MINIBLOCKDATABYTES; i++)
    {
        for (int j = 0; j < XAS_MINIBLOCKGROUP; j++)
        {
            pDst[j] = pSrc[i + j * XAS_MINIBLOCKBYTES];
        }

        pDst += XAS_MINIBLOCKGROUP;
    }
}

void Xas1Enc::EncodeBlock(float* src, unsigned char* dest, int channels)
{
    float output;
    float diff;
    float tempfloat;
    float sample;
    float minerror;
    float maxerror[4];
    int smp1;
    int smp2;
    int filter = 0;
    int shift;
    int i;
    int j;
    int dstq;
    int tempint;
    int s2;
    int s1;
    float d2;
    float d1;
    float dstarr[4][30];
    float* pTempSrc;
    unsigned char* pTempDest = dest;
    unsigned char encodedBlocks[XAS_BLOCKBYTES];

    for (int chan = 0; chan < channels; chan++)
    {
        unsigned char* pBlockGroup = encodedBlocks;
        pTempSrc = src;

        for (int semiblock = 0; semiblock < XAS_MINIBLOCKGROUP; semiblock++)
        {
            minerror = 1e9;

            s2 = static_cast<int>(pTempSrc[chan] * 32767.0 + 8.0) & ~0xf;
            pTempSrc += channels;
            s1 = static_cast<int>(pTempSrc[chan] * 32767.0 + 8.0) & ~0xf;

            if (s2 > 32752)
            {
                s2 = 32752;
            }

            if (s1 > 32752)
            {
                s1 = 32752;
            }

            pTempSrc += channels;

            for (j = 0; j < 4; j++)
            {
                smp1 = s1;
                smp2 = s2;
                maxerror[j] = 0.0f;

                for (i = 0; i < 30; i++)
                {
                    sample = pTempSrc[chan + i * channels] * 32767;
                    sample += sFilterPairs[j][0] * smp1 + sFilterPairs[j][1] * smp2;
                    dstarr[j][i] = pTempSrc[chan + i * channels] * (float)32767.0;
                    tempfloat = sample > 0 ? sample : -sample;

                    if (maxerror[j] < tempfloat)
                    {
                        maxerror[j] = tempfloat;
                    }

                    smp2 = smp1;
                    smp1 = (int)(pTempSrc[chan + i * channels] * 32767.0);
                }

                if (minerror > maxerror[j])
                {
                    minerror = maxerror[j];
                    filter = j;
                }

                if (j == 0)
                {
                    if (maxerror[0] <= 7.0)
                    {
                        filter = 0;
                        break;
                    }
                }
            }

            pTempSrc += 30 * channels;
            tempint = (int)maxerror[filter];

            if (tempint > 32767)
            {
                tempint = 32767;
            }

            if (tempint < -32768)
            {
                tempint = -32768;
            }

            i = 1 << 14;

            for (shift = 0; shift < 12; shift++, i >>= 1)
            {
                if ((tempint + (i >> 3)) & i)
                {
                    break;
                }
            }

            d2 = (float)s2;
            d1 = (float)s1;
            s2 |= filter;
            s1 |= shift;
            pBlockGroup[0] = static_cast<unsigned char>(s2 & 0xff);
            pBlockGroup[1] = static_cast<unsigned char>((s2 >> 8) & 0xff);
            pBlockGroup[2] = static_cast<unsigned char>(s1 & 0xff);
            pBlockGroup[3] = static_cast<unsigned char>((s1 >> 8) & 0xff);
            pBlockGroup += 4;

            for (i = 0; i < 30; i++)
            {
                output = dstarr[filter][i] + sFilterPairs[filter][0] * d1 + sFilterPairs[filter][1] * d2;
                tempint = (int)(output * (1L << shift));
                dstq = (tempint + 0x0800) & ~0x0fff;

                if (dstq > 32767)
                {
                    dstq = 32767;
                }

                if (dstq < -32768)
                {
                    dstq = -32768;
                }

                if (i & 1)
                {
                    *pBlockGroup |= (dstq >> 12) & 0x0f;
                    pBlockGroup++;
                }
                else
                {
                    *pBlockGroup = static_cast<unsigned char>((dstq >> 8) & 0xf0);
                }

                tempfloat = float(dstq >> shift);
                diff = tempfloat - (sFilterPairs[filter][0] * d1 + sFilterPairs[filter][1] * d2);
                d2 = d1;
                d1 = diff;
            }
        }

        CustomInterleaveXAS(encodedBlocks, pTempDest);
        pTempDest += XAS_BLOCKBYTES;
    }
}

int Xas1Enc::Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    int framesEncoded = 0;
    *pBytesEncoded = 0;

    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    if (mResidualSamples)
    {
        int framesForResidue = numSamplesIn < XAS_BLOCKSAMPLES - mResidualSamples ? numSamplesIn : XAS_BLOCKSAMPLES - mResidualSamples;
        float* mResidueBufferPtr = mResidueBuffer + (mResidualSamples * GetChannels());
        memcpy(mResidueBufferPtr, pSrc, framesForResidue * GetChannels() * sizeof(float));
        pSrc += framesForResidue * GetChannels();
        mResidualSamples += framesForResidue;
        numSamplesIn -= framesForResidue;

        if (mResidualSamples == XAS_BLOCKSAMPLES)
        {
            mResidueBufferPtr = mResidueBuffer;
            EncodeBlock(mResidueBufferPtr, pDst, GetChannels());
            pDst += XAS_BLOCKBYTES * GetChannels();
            framesEncoded += XAS_BLOCKSAMPLES;
            *pBytesEncoded += XAS_BLOCKBYTES * GetChannels();
            mResidualSamples = 0;
        }
    }

    int multiples = numSamplesIn / XAS_BLOCKSAMPLES;

    for (int i = 0; i < multiples; i++)
    {
        EncodeBlock(pSrc, pDst, GetChannels());
        pSrc += XAS_BLOCKSAMPLES * GetChannels();
        pDst += XAS_BLOCKBYTES * GetChannels();
        framesEncoded += XAS_BLOCKSAMPLES;
        numSamplesIn -= XAS_BLOCKSAMPLES;
        *pBytesEncoded += XAS_BLOCKBYTES * GetChannels();
    }

    if (numSamplesIn)
    {
        mResidualSamples = numSamplesIn;
        memcpy(mResidueBuffer, pSrc, mResidualSamples * GetChannels() * sizeof(float));
        pSrc += mResidualSamples * GetChannels();
    }

    return framesEncoded;
}

int Xas1Enc::Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    if ((mResidualSamples > 0) && (mResidualSamples < XAS_BLOCKSAMPLES))
    {
        int j, k;
        int frames = mResidualSamples;
        float* pSrc = mResidueBuffer;
        float* pPadSrc = pSrc + ((mResidualSamples - 1) * GetChannels());
        float* pPadLocation = pPadSrc + GetChannels();

        for (j = mResidualSamples; j < XAS_BLOCKSAMPLES; j++)
        {
            for (k = 0; k < GetChannels(); k++)
            {
                *pPadLocation = pPadSrc[k];
                pPadLocation++;
            }
        }

        EncodeBlock(pSrc, pDst, GetChannels());
        mResidualSamples = 0;
        *pBytesEncoded = XAS_BLOCKBYTES * GetChannels();
        return frames;
    }

    *pBytesEncoded = 0;
    return 0;
}
