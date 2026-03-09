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

#include "cmn\encoderhelper.h"
#include "cmn\fileio.h"
#include "system.h"
#include "simex\simex.h"
#include "cmn\isimex.h"
#include <new>

EncoderHelper* EncoderHelper::CreateInstance(void* psound, int guid)
{
    SSOUND* pss = (SSOUND*)psound;
    EncoderHelper* pThis = (EncoderHelper*)Allocator::Alloc(sizeof(EncoderHelper));
    pThis = new (pThis) EncoderHelper();
    System* pSndSystem = System::GetInstance();
    pSndSystem->Lock();
    pThis->mpEncoderRegistry = pSndSystem->GetEncoderRegistry();
    void* encoderHandle = pThis->mpEncoderRegistry->GetEncoderHandle(guid);
    pThis->mpEncoder = pThis->mpEncoderRegistry->EncoderExtendedFactory(encoderHandle, pss->numchannels, pss->samplerate, pSndSystem);
    pSndSystem->Unlock();
    pThis->mpEncodedDataBuffer = 0;
    pThis->mpFlushedData = 0;
    pThis->mEncodedDataBufferSize = 0;
    pThis->mpEncodedSeekBuffer = 0;
    pThis->mpFlushedSeek = 0;
    pThis->mEncodedSeekBufferSize = 0;
    pThis->mCanFlush = 0;
    return pThis;
}

float EncoderHelper::GetAverageDataRate()
{
    return mpEncoder->GetAverageDataRate();
}

bool EncoderHelper::IsSeekable()
{
    return mpEncoder->IsSeekable();
}

int EncoderHelper::Encode(short* pSampleData[], unsigned char** pEncodedData, int numSamples, int* bytesEncoded, void* psound, unsigned char** ppSeekData, int* pSeekDataBytes)
{
    int minBufferSize = mpEncoder->GetEncodeMemoryRequired(numSamples);

    if (mEncodedDataBufferSize < minBufferSize)
    {
        if (mEncodedDataBufferSize)
        {
            Allocator::Free(mpEncodedDataBuffer);
        }

        mpEncodedDataBuffer = (unsigned char*)Allocator::Alloc(minBufferSize);
        mEncodedDataBufferSize = minBufferSize;
    }

    int minSeekDataSize = mpEncoder->GetSeekMemoryRequired(numSamples);

    if (mEncodedSeekBufferSize < minSeekDataSize)
    {
        if (mEncodedSeekBufferSize)
        {
            Allocator::Free(mpEncodedSeekBuffer);
        }

        mpEncodedSeekBuffer = (unsigned char*)Allocator::Alloc(minSeekDataSize);
        mEncodedSeekBufferSize = minSeekDataSize;
    }

    SSOUND* pss = (SSOUND*)psound;

    if (pss->bitrate >= 0 && pss->bitrate <= 100)
    {
        float vbr = pss->bitrate * 0.009999999776482582;
        mpEncoder->SetVbrQuality(vbr);
    }
    else if (pss->bitrate > 100)
    {
        mpEncoder->SetCbrRate(pss->bitrate);
    }
    else
    {
        mpEncoder->SetVbrQuality(0.9f);
    }

    int frameEncoded = 0;
    frameEncoded = mpEncoder->Encode(pSampleData, mpEncodedDataBuffer, numSamples, bytesEncoded, mpEncodedSeekBuffer, pSeekDataBytes);
    *pEncodedData = mpEncodedDataBuffer;
    *ppSeekData = mpEncodedSeekBuffer;
    mpFlushedData = &mpEncodedDataBuffer[*bytesEncoded];

    if (mpEncodedSeekBuffer)
    {
        mpFlushedSeek = &mpEncodedSeekBuffer[*pSeekDataBytes];
    }

    mCanFlush = true;
    return frameEncoded;
}

int EncoderHelper::Flush(unsigned char** pEncodedData, int* bytesEncoded, unsigned char** ppSeekData, int* pSeekDataBytes)
{
    int frameEncoded = 0;

    if (mCanFlush)
    {
        frameEncoded = mpEncoder->Flush(mpFlushedData, bytesEncoded, mpFlushedSeek, pSeekDataBytes);
        *pEncodedData = mpFlushedData;
        *ppSeekData = mpFlushedSeek;
        mCanFlush = false;
    }
    else
    {
        SIMEXI_setlasterr("EncoderHelper::Flush - Can only be called after some data is encoded.");
        return -1;
    }

    return frameEncoded;
}

void EncoderHelper::Release()
{
    mpEncoder->Release();

    if (mpEncodedDataBuffer)
    {
        Allocator::Free(mpEncodedDataBuffer);
    }

    if (mpEncodedSeekBuffer)
    {
        Allocator::Free(mpEncodedSeekBuffer);
    }

    Allocator::Free(this);
}
