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

#include "decoder.h"
#include "ibase.h"

static const int DECODER_MAX_SAMPLES = 1024;

DecoderExtended* DecoderExtended::CreateInstance(System* pSystem, unsigned int numChannels)
{
    unsigned int memRequired = sizeof(DecoderExtended);
    unsigned int alignmentRequired = 0;
    unsigned int sampleBufferAlignment;
    unsigned int sampleBufferInstanceSize = SampleBuffer::GetSize(numChannels, numChannels, DECODER_MAX_SAMPLES, &sampleBufferAlignment, pSystem);
    alignmentRequired = alignmentRequired > sampleBufferAlignment ? alignmentRequired : sampleBufferAlignment;
    LinearAllocAddSize(memRequired, sampleBufferInstanceSize, sampleBufferAlignment);
    unsigned int storageSize = SampleBuffer::CalculateStorageSize(numChannels, DECODER_MAX_SAMPLES);
    alignmentRequired = alignmentRequired > 128 ? alignmentRequired : 128;
    LinearAllocAddSize(memRequired, storageSize, 128);
    DecoderExtended* pThis;
    pSystem->New2(&pThis, memRequired, alignmentRequired);
    uintptr_t pMemBlock;
    pMemBlock = reinterpret_cast<uintptr_t>(pThis) + sizeof(*pThis);
    LinearAlloc(pThis->mpSampleBuffer, pMemBlock, sampleBufferInstanceSize, sampleBufferAlignment);
    LinearAlloc(pThis->mpSampleBufferStorage, pMemBlock, storageSize, 128);
    SampleBuffer::CreateInstance(numChannels, numChannels, DECODER_MAX_SAMPLES, reinterpret_cast<void*>(pThis->mpSampleBuffer), pThis->mpSampleBufferStorage, pSystem);
    return pThis;
}

void DecoderExtended::Release()
{
    System* pSystem = mpDecoder->GetSystem();
    mpDecoder->Release();
    pSystem->Delete(this);
}

Decoder::RequestHandle DecoderExtended::Feed(void* pSrc, int numSamples, Decoder::FeedType feedType)
{
    return mpDecoder->Feed(pSrc, numSamples, feedType, 0, 0, 0);
}

int DecoderExtended::Decode(float* pDst[], int samples)
{
    int samplesLeft = samples;
    unsigned int numChannels = mpDecoder->GetChannels();
    int totalSamplesDecoded = 0;

    while (samplesLeft > 0)
    {
        int samplesToDecode = samplesLeft < DECODER_MAX_SAMPLES ? samplesLeft : DECODER_MAX_SAMPLES;
        int samplesDecoded = mpDecoder->Decode(mpSampleBuffer, samplesToDecode);

        for (unsigned int chan = 0; chan < numChannels; chan++)
        {
            float* pDecodedData = mpSampleBuffer->LockChannel(chan);
            memcpy(&pDst[chan][totalSamplesDecoded], pDecodedData, samplesDecoded * sizeof(float));
            mpSampleBuffer->UnlockChannel(chan);
        }

        samplesLeft -= samplesDecoded;
        totalSamplesDecoded += samplesDecoded;

        if (samplesDecoded < samplesToDecode)
        {
            break;
        }
    }

    return totalSamplesDecoded;
}

int DecoderExtended::Decode(float* pDst, int samples)
{
    int samplesLeft = samples;
    unsigned int numChannels = mpDecoder->GetChannels();
    int ret = 0;

    while (samplesLeft > 0)
    {
        int samplesToDecode = samplesLeft < DECODER_MAX_SAMPLES ? samplesLeft : DECODER_MAX_SAMPLES;
        int samplesDecoded = mpDecoder->Decode(mpSampleBuffer, samplesToDecode);

        for (unsigned int chan = 0; chan < numChannels; chan++)
        {
            float* pDecodedData = mpSampleBuffer->LockChannel(chan);

            for (int i = 0; i < samplesDecoded; i++)
            {
                pDst[chan + (i * numChannels)] = pDecodedData[i];
            }

            mpSampleBuffer->UnlockChannel(chan);
        }

        samplesLeft -= samplesDecoded;
        pDst += (samplesDecoded * numChannels);
        ret += samplesDecoded;

        if (samplesDecoded < samplesToDecode)
        {
            break;
        }
    }

    return ret;
}

int DecoderExtended::Decode(short* pDst, int samples)
{
    int samplesLeft = samples;
    unsigned int numChannels = mpDecoder->GetChannels();
    int ret = 0;

    while (samplesLeft > 0)
    {
        int samplesToDecode = samplesLeft < DECODER_MAX_SAMPLES ? samplesLeft : DECODER_MAX_SAMPLES;
        int samplesDecoded = mpDecoder->Decode(mpSampleBuffer, samplesToDecode);

        if (samplesDecoded <= 0)
        {
            break;
        }

        for (unsigned int chan = 0; chan < numChannels; chan++)
        {
            float* pDecodedData = mpSampleBuffer->LockChannel(chan);

            for (int i = 0; i < samplesDecoded; i++)
            {
                float samplefloat = pDecodedData[i] * 32768.0f;
                short sampleshort;

                if (samplefloat > 32767.0f)
                {
                    sampleshort = 32767;
                }
                else if (samplefloat < -32768.0f)
                {
                    sampleshort = -32768;
                }
                else
                {
                    sampleshort = (short)(samplefloat);
                }

                pDst[chan + (i * numChannels)] = sampleshort;
            }

            mpSampleBuffer->UnlockChannel(chan);
        }

        samplesLeft -= samplesDecoded;
        pDst += (samplesDecoded * numChannels);
        ret += samplesDecoded;

        if (samplesDecoded < samplesToDecode)
        {
            break;
        }
    }

    return ret;
}

int DecoderExtended::Decode(short* pDst[], int samples)
{
    int retVal = 0;
    int decodeResult;
    int count;
    unsigned int chan;

    while (samples > 0)
    {
        count = samples < DECODER_MAX_SAMPLES ? samples : DECODER_MAX_SAMPLES;
        decodeResult = mpDecoder->Decode(mpSampleBuffer, count);

        if (decodeResult <= 0)
        {
            break;
        }

        for (chan = 0; chan < mpDecoder->GetChannels(); chan++)
        {
            float* pDecodedData = mpSampleBuffer->LockChannel(chan);
            TranslateF32ToS16(pDecodedData, &pDst[chan][retVal], decodeResult);
            mpSampleBuffer->UnlockChannel(chan);
        }

        retVal += decodeResult;
        samples -= decodeResult;

        if (decodeResult < count)
        {
            break;
        }
    }

    return retVal;
}

void DecoderExtended::TranslateF32ToS16(void* pSrc, void* pDst, int numSamples)
{
    float* pF = static_cast<float*>(pSrc);
    short* pI = static_cast<short*>(pDst);

    for (int i = 0; i < numSamples; i++)
    {
        float samplefloat = pF[i] * 32768.0f;

        if (samplefloat > 32767.0f)
        {
            pI[i] = 32767;
        }
        else if (samplefloat < -32768.0f)
        {
            pI[i] = -32768;
        }
        else
        {
            pI[i] = (short)(samplefloat);
        }
    }
}
