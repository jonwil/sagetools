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
#include "endian.h"

void process_raw_block(MXAPACKET16* pxap)
{
    if (*pxap->psrc != 238)
    {
        return;
    }

    short temp;
    unsigned int i;
    pxap->psrc++;
    ENDIAN::Put(true, temp, *(short*)pxap->psrc);
    pxap->sample1 = temp;
    pxap->psrc += 2;
    ENDIAN::Put(true, temp, *(short*)pxap->psrc);
    pxap->sample2 = temp;
    pxap->psrc += 2;

    for (i = 0; i < 28; i++)
    {
        ENDIAN::Put(true, temp, *(short*)pxap->psrc);
        *pxap->pdst++ = temp;
        pxap->psrc += 2;
    }
}

struct XAFILTERPAIR
{
    short a;
    short b;
};

XAFILTERPAIR xafp[4] = { {0, 0}, {240, 0}, {460, -208}, {392, -220} };

void decxa16c(MXAPACKET16* xap)
{
    unsigned char* ptmpSrc = xap->psrc;
    int sample;
    int mul1;
    int mul2;
    int shift;
    short* pnewdst;

    while (xap->numframes > 0)
    {
        if (*ptmpSrc == 238)
        {
            process_raw_block(xap);
            xap->numframes -= 28;
        }
        else
        {
            xap->numframes -= 28;
            shift = (int)*xap->psrc >> 4;
            mul1 = xafp[shift].a;
            mul2 = xafp[shift].b;
            shift = (*xap->psrc & 15) + 8;
            pnewdst = xap->pdst + 28;
            xap->psrc++;

            while (xap->pdst < pnewdst)
            {
                sample = *xap->psrc >> 4;
                sample = sample << 28 >> shift;
                sample += mul1 * xap->sample1 + mul2 * xap->sample2;
                sample >>= 8;

                if (sample < -32768)
                {
                    sample = -32768;
                }
                else if (sample > 32767)
                {
                    sample = 32767;
                }

                *xap->pdst = sample;
                xap->sample2 = sample;
                sample = *xap->psrc << 28 >> shift;
                sample += mul1 * xap->sample2 + mul2 * xap->sample1;
                sample >>= 8;

                if (sample < -32768)
                {
                    sample = -32768;
                }
                else if (sample > 32767)
                {
                    sample = 32767;
                }

                xap->pdst[1] = sample;
                xap->sample1 = sample;
                xap->psrc++;
                xap->pdst += 2;
            }
        }

        ptmpSrc = xap->psrc;
    }
}

void* CEAXABLKDec::operator new(size_t size)
{
    return CODANew(size);
}

void CEAXABLKDec::operator delete(void* ptr)
{
    CODADelete(ptr);
}

CEAXABLKDec::CEAXABLKDec()
{
    xav.sampledatasize = 0;
    xav.numsamples = 0;
    xav.residual = 0;
    xap.sample1 = 0;
    xap.sample2 = 0;
}

int CEAXABLKDec::Feed(void* pSampleData, int sampleDataSize, int numSamples)
{
    if (!pSampleData)
    {
        return -1;
    }

    if (xav.numsamples)
    {
        return -1;
    }
    else
    {
        xav.numsamples = numSamples;
    }

    xav.sampledatasize = sampleDataSize;
    xap.psrc = (unsigned char*)pSampleData;
    return 0;
}

int CEAXABLKDec::Decode(short* pDstBuf[], int numSamples)
{
    int retVal = 0;
    int framesleft = 0;
    xap.pdst = *pDstBuf;

    if (!xav.numsamples)
    {
        return 0;
    }

    int maxsamples = numSamples < xav.numsamples ? numSamples : xav.numsamples;
    int i;
    int tmpframecnt;

    if (xav.residual)
    {
        if (xav.residual <= maxsamples)
        {
            tmpframecnt = xav.residual;
        }
        else
        {
            tmpframecnt = maxsamples;
        }

        for (i = 0; i < tmpframecnt; i++)
        {
            *xap.pdst++ = *xav.presidue++;
        }

        xav.numsamples -= tmpframecnt;
        xav.residual -= tmpframecnt;
        maxsamples -= tmpframecnt;
        retVal += tmpframecnt;
    }

    i = 28 * (maxsamples / 28);
    xap.numframes = i;
    framesleft = maxsamples - xap.numframes;
    short* ptmpDst;

    if (xap.numframes > 0)
    {
        decxa16c(&xap);
        xav.numsamples -= i;
        retVal += i;
    }

    if (framesleft > 0)
    {
        ptmpDst = xap.pdst;
        xap.pdst = &xav.xablock[2];
        xap.numframes = framesleft;
        decxa16c(&xap);
        xav.numsamples -= framesleft;
        xav.presidue = &xap.pdst[xap.numframes];
        xav.residual = -xap.numframes;
        xap.pdst -= 28;

        for (i = 0; i < framesleft; i++)
        {
            ptmpDst[i] = xap.pdst[i];
        }

        retVal += framesleft;
    }

    if (xav.numsamples <= 0)
    {
        xav.residual = 0;
    }

    return retVal;
}

XA16STATE CEAXABLKDec::GetState()
{
    XA16STATE xtmp;
    xtmp.sample1 = xap.sample1;
    xtmp.sample2 = xap.sample2;
    return xtmp;
}

void CEAXABLKDec::SetState(XA16STATE* pxav)
{
    xap.sample1 = pxav->sample1;
    xap.sample2 = pxav->sample2;
}
