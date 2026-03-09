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

#include "encoder.h"

static const int ENCODER_MAX_CHANNELS = 64;
static const int ENCODER_MAX_SAMPLES = 256;
static const int ENCODER_MAX_BUFSIZE = ENCODER_MAX_SAMPLES * ENCODER_MAX_CHANNELS;

void Encoder::Release()
{
    mpSystem->Delete(this);
}

int Encoder::GetDataRateOverhead()
{
    return 10 * 1024 * GetChannels();
}

float Encoder::GetAverageDataRate()
{
    return mAverageDataRate;
}

int Encoder::GetEncodeMemoryRequired(int numSamples)
{
    return (int)(mAverageDataRate * (float)numSamples / (float)mSampleRate) + this->GetDataRateOverhead();
}

int Encoder::GetSeekMemoryRequired(int numSamples)
{
    return 0;
}

void Encoder::SetVbrQuality(float quality)
{
    mVbrQuality = quality;
    mBitRateManagement = BITRATEMANAGEMENT_USINGVBR;
}

EncoderExtended* EncoderExtended::CreateInstance(System* pSystem)
{
    EncoderExtended* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        return 0;
    }

    return pThis;
}

void EncoderExtended::Release()
{
    System* pSystem = mpEncoder->GetSystem();
    mpEncoder->Release();
    pSystem->Delete(this);
}

int EncoderExtended::GetDataRateOverhead()
{
    return mpEncoder->GetDataRateOverhead();
}

float EncoderExtended::GetAverageDataRate()
{
    return mpEncoder->GetAverageDataRate();
}

int EncoderExtended::GetEncodeMemoryRequired(int numSamples)
{
    return mpEncoder->GetEncodeMemoryRequired(numSamples + ENCODER_MAX_SAMPLES);
}

int EncoderExtended::GetSeekMemoryRequired(int numSamples)
{
    return mpEncoder->GetSeekMemoryRequired(numSamples + ENCODER_MAX_SAMPLES);
}

bool EncoderExtended::IsSeekable()
{
    return mpEncoder->IsSeekable();
}

void EncoderExtended::SetVbrQuality(float quality)
{
    mpEncoder->SetVbrQuality(quality);
}

void EncoderExtended::SetCbrRate(int bitsPerSecond)
{
    mpEncoder->SetCbrRate(bitsPerSecond);
}

int EncoderExtended::Encode(float* pSampleData, unsigned char* pEncodedData, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    return mpEncoder->Encode(pSampleData, pEncodedData, numSamplesIn, pBytesEncoded, pSeekData, pSeekDataBytes);
}

int EncoderExtended::Encode(short* pSampleData[], unsigned char* pEncodedData, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    int retVal = 0;
    float temp[ENCODER_MAX_BUFSIZE];
    int samplesThisBlock;
    int encodedBytes;
    int seekDataBytes;
    short* ptempSampleData[ENCODER_MAX_CHANNELS];

    for (int k = 0; k < mpEncoder->GetChannels(); k++)
    {
        ptempSampleData[k] = &pSampleData[k][0];
    }

    *pBytesEncoded = 0;
    *pSeekDataBytes = 0;

    while (numSamplesIn > 0)
    {
        samplesThisBlock = numSamplesIn < ENCODER_MAX_SAMPLES ? numSamplesIn : ENCODER_MAX_SAMPLES;
        TranslateS16ToF32(ptempSampleData, temp, mpEncoder->GetChannels(), samplesThisBlock);
        retVal += mpEncoder->Encode(temp, pEncodedData, samplesThisBlock, &encodedBytes, pSeekData, &seekDataBytes);
        *pBytesEncoded += encodedBytes;
        pEncodedData += encodedBytes;
        *pSeekDataBytes += seekDataBytes;
        pSeekData = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(pSeekData) + seekDataBytes);
        numSamplesIn -= ENCODER_MAX_SAMPLES;
    }

    return retVal;
}

void EncoderExtended::TranslateS16ToF32(short* pSrc[], float* pDst, int channels, int numSamples)
{
    int i;
    int j;

    for (i = 0; i < numSamples; i++)
    {
        for (j = 0; j < channels; j++)
        {
            *pDst = static_cast<float>(*pSrc[j] * (1.0f / 32768.0f));
            pDst++;
            pSrc[j]++;
        }
    }

    for (i = numSamples; i < ENCODER_MAX_SAMPLES; i++)
    {
        for (j = 0; j < channels; j++)
        {
            *pDst = (float)*(pSrc[j] - 1) * (1.0f / 32768.0f);
            pDst++;
        }
    }
}

int EncoderExtended::Flush(unsigned char* pEncodedData, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    return mpEncoder->Flush(pEncodedData, pBytesEncoded, pSeekData, pSeekDataBytes);
}
