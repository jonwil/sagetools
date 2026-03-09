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

#include <string.h>
#include "system.h"
#include "decoderregistry.h"
#include "decoders\decealayer32.h"
#include "encoders\encealayer32.h"
#include "endian.h"
#include "decoders\ealayer31\ealayer32block.h"
#include "encoders\cmn\layer3shared.h"
#include "encoders\cmn\encealayer3blockbuilder.h"
#include "encoders\lame396\lame.h"
#include "encoders\lame396\lame_global_flags.h"
#include "encoders\mp3toea.h"

EncoderDesc EaLayer32PcmEnc::sEncoderDesc = { GUID, CreateInstance, 0, 576 * 3, 0, true };
EncoderDesc EaLayer32SpikeEnc::sEncoderDesc = { GUID, CreateInstance, 0, 576 * 3, 0, true };

EncoderDesc* EaLayer32PcmEnc::GetEncoderDesc()
{
    System* pSndSystem = System::GetInstance();
    DecoderRegistry* pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    pDecoderRegistry->RegisterDecoder(EaLayer32SpikeDec::GetDecoderDesc());
    return &sEncoderDesc;
}

EncoderDesc* EaLayer32SpikeEnc::GetEncoderDesc()
{
    System* pSndSystem = System::GetInstance();
    DecoderRegistry* pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    pDecoderRegistry->RegisterDecoder(EaLayer32SpikeDec::GetDecoderDesc());
    return &sEncoderDesc;
}

Encoder* EaLayer32PcmEnc::CreateInstance(int numChannels, int sampleRate, System* pSystem)
{
    char startupType = 0;
    return EaLayer32Enc::CreateInstance(numChannels, sampleRate, startupType, pSystem);
}

Encoder* EaLayer32SpikeEnc::CreateInstance(int numChannels, int sampleRate, System* pSystem)
{
    char startupType = 1;
    return EaLayer32Enc::CreateInstance(numChannels, sampleRate, startupType, pSystem);
}

Encoder* EaLayer32Enc::CreateInstance(int numChannels, int sampleRate, char startupType, System* pSystem)
{
    EaLayer32Enc* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        goto abort;
    }

    pThis->mNumChannelPairs = (numChannels + 1) / 2;
    pThis->mAverageDataRate = static_cast<float>(EALAYER3_AVERAGEDATARATE * numChannels);
    pThis->mVbrQuality = 0.0f;
    pThis->mCbrRate = 4000;
    pThis->mIsBuilderInitDone = false;
    pThis->mStartupType = startupType;
    pThis->mpBlockBuilder = 0;
    pThis->mpTempSampleBuffer = 0;
    pThis->mpBlockBuilder = static_cast<EaLayer32BlockBuilder*>(pSystem->Alloc(pThis->mNumChannelPairs * sizeof(EaLayer32BlockBuilder)));

    if (!pThis->mpBlockBuilder)
    {
        goto abort;
    }

    pThis->mpTempSampleBuffer = static_cast<float*>(pSystem->Alloc(2 * TEMP_BUFFER_SIZE_SAMPLES * sizeof(float)));

    if (!pThis->mpTempSampleBuffer)
    {
        goto abort;
    }

    return static_cast<Encoder*>(pThis);

abort:
    if (pThis && pThis->mpBlockBuilder)
    {
        pSystem->Delete(pThis->mpBlockBuilder);
    }

    if (pThis && pThis->mpTempSampleBuffer)
    {
        pSystem->Delete(pThis->mpTempSampleBuffer);
    }

    if (pThis)
    {
        pSystem->Delete(pThis);
    }

    return 0;
}

void EaLayer32Enc::InitBuilders()
{
    mIsBuilderInitDone = true;
    EaLayer32BlockBuilder::StartupMode startUpMode = static_cast<EaLayer32BlockBuilder::StartupMode>(mStartupType);
    int totalChannels = GetChannels();
    int channelsDone = 0;

    for (int i = 0; i < mNumChannelPairs; i++)
    {
        int numChannels = Min(totalChannels - channelsDone, 2);
        mpBlockBuilder[i].Init(numChannels, GetSampleRate(), mVbrQuality, startUpMode, EALAYER32_CROSSFADE_SAMPLES, GetSystem());
        channelsDone += numChannels;
    }
}

void EaLayer32Enc::Reset()
{
    if (!mIsBuilderInitDone)
    {
        InitBuilders();
    }

    for (int i = 0; i < mNumChannelPairs; i++)
    {
        mpBlockBuilder[i].Reset();
    }
}

void EaLayer32Enc::Release()
{
    if (mIsBuilderInitDone)
    {
        mIsBuilderInitDone = false;

        for (int i = 0; i < mNumChannelPairs; i++)
        {
            mpBlockBuilder[i].Release();
        }
    }

    if (mpBlockBuilder)
    {
        GetSystem()->Free(mpBlockBuilder);
    }

    if (mpTempSampleBuffer)
    {
        GetSystem()->Free(mpTempSampleBuffer);
    }

    GetSystem()->Delete(this);
}

inline float EaLayer32Enc::ScaleSample(const float& value)
{
    static const float scaleFactor = 32768.0f;
    static const float maxValue = scaleFactor - 1;
    static const float minValue = -scaleFactor;
    float scaledValue = value * scaleFactor;

    if (maxValue < scaledValue)
    {
        return maxValue;
    }
    else if (scaledValue < minValue)
    {
        return minValue;
    }

    return scaledValue;
}

inline void EaLayer32Enc::SampleCopyDeinterleaveScaleClip(float* pDst, int dstSize, int numChannels, float* pSrc, int numSamples, int stride)
{
    for (int sample = 0; sample < numSamples; sample++)
    {
        for (int channel = 0; channel < numChannels; channel++)
        {
            pDst[channel * dstSize + sample] = ScaleSample(pSrc[sample * stride + channel]);
        }
    }
}

void EaLayer32Enc::FeedBlockBuilders(float* pSampleData, int numSamples)
{
    if (!mIsBuilderInitDone)
    {
        InitBuilders();
    }

    int samplesFed = 0;

    while (samplesFed < numSamples)
    {
        int samplesRemaining = numSamples - samplesFed;
        int currentSamples = Min(samplesRemaining, TEMP_BUFFER_SIZE_SAMPLES);
        int totalChannels = GetChannels();
        int channelsDone = 0;

        for (int i = 0; i < mNumChannelPairs; i++)
        {
            int numChannels = Min(totalChannels - channelsDone, 2);
            float* pSrc = &pSampleData[samplesFed * totalChannels + channelsDone];
            SampleCopyDeinterleaveScaleClip(mpTempSampleBuffer, TEMP_BUFFER_SIZE_SAMPLES, numChannels, pSrc, currentSamples, totalChannels);
            float* pSrc0 = &mpTempSampleBuffer[0];
            float* pSrc1 = 0;

            if (numChannels == 2)
            {
                pSrc1 = &mpTempSampleBuffer[TEMP_BUFFER_SIZE_SAMPLES];
            }

            mpBlockBuilder[i].Feed(pSrc0, pSrc1, currentSamples);
            channelsDone += numChannels;
        }

        samplesFed += currentSamples;
    }
}

void EaLayer32Enc::FlushBlockBuilders()
{
    for (int i = 0; i < mNumChannelPairs; i++)
    {
        mpBlockBuilder[i].Flush();
    }
}

bool EaLayer32Enc::IsBlockSetReady()
{
    bool result = true;

    for (int i = 0; i < mNumChannelPairs; i++)
    {
        result &= mpBlockBuilder[i].IsBlockAvailable();
    }

    return result;
}

void EaLayer32Enc::WriteBlockSet(unsigned char* pDst, int* pBytesWritten, int* pSamplesWritten)
{
    int totalBytesWritten = 0;
    int samplesWritten = 0;

    for (int i = 0; i < mNumChannelPairs; i++)
    {
        int bytes;
        int samples;
        mpBlockBuilder[i].GetNextBlockInfo(&samples, &bytes);
        mpBlockBuilder[i].WriteNextBlock(&pDst[totalBytesWritten]);
        totalBytesWritten += bytes;
        samplesWritten = samples;
    }

    *pBytesWritten = totalBytesWritten;
    *pSamplesWritten = samplesWritten;
}

int EaLayer32Enc::Encode(float* pSrc, unsigned char* pDst, int samples, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    FeedBlockBuilders(pSrc, samples);
    int totalBytesWritten = 0;
    int totalSamplesWritten = 0;
    bool isSeekable = (pSeekData != 0 && pSeekDataBytes != 0);
    short* pCurrentSeekData = static_cast<short*>(pSeekData);

    if (pSeekDataBytes != NULL)
    {
        *pSeekDataBytes = 0;
    }

    while (IsBlockSetReady())
    {
        int bytesWritten;
        int samplesWritten;
        WriteBlockSet(&pDst[totalBytesWritten], &bytesWritten, &samplesWritten);
        totalBytesWritten += bytesWritten;
        totalSamplesWritten += samplesWritten;

        if (isSeekable)
        {
            short packetLength = static_cast<short>(bytesWritten);
            ENDIAN::PutUB(*pCurrentSeekData++, packetLength);
            *pSeekDataBytes += sizeof(short);
        }
    }

    *pBytesEncoded = totalBytesWritten;
    return totalSamplesWritten;
}

int EaLayer32Enc::Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    FlushBlockBuilders();
    float* pSrc = 0;
    int samples = 0;
    int samplesFlushed = Encode(pSrc, pDst, samples, pBytesEncoded, pSeekData, pSeekDataBytes);
    Reset();
    return samplesFlushed;
}

int EaLayer32Enc::GetSeekMemoryRequired(int numSamples)
{
    int numBlocks = numSamples / 576;
    static const int EXTRA_PADDING = 8 * 1024;
    return numBlocks * 3072 + EXTRA_PADDING;
}
