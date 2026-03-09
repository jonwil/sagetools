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

#ifndef ENCEALAYER32_H
#define ENCEALAYER32_H

#include "encoder.h"

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
struct EALAYER3STRUCT;
struct MPEGAUDIOHDR;
class EaLayer32BlockBuilder;

class EaLayer32Enc : public Encoder
{
public:
    virtual int Encode(float* pSrc, unsigned char* pDst, int samples, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int GetSeekMemoryRequired(int numSamples);
    void Reset();
    virtual void Release();

protected:
    static Encoder* CreateInstance(int numChannels, int sampleRate, char startupType, System* pSystem);

private:
    static const int TEMP_BUFFER_SIZE_SAMPLES = 1024;
    static const int EALAYER32_CROSSFADE_SAMPLES = 47;
    static const int EALAYER3_AVERAGEDATARATE = 384000 / 8;

    static float ScaleSample(const float& value);
    static void SampleCopyDeinterleaveScaleClip(float* pDst, int dstSize, int numChannels, float* pSrc, int numSamples, int stride);
    void InitBuilders();
    void FeedBlockBuilders(float* pSampleData, int numSamples);
    void FlushBlockBuilders();
    bool IsBlockSetReady();
    void WriteBlockSet(unsigned char* pDst, int* pBytesWritten, int* pSamplesWritten);

    float* mpTempSampleBuffer;
    EaLayer32BlockBuilder* mpBlockBuilder;
    int mNumChannelPairs;
    char mStartupType;
    bool mIsBuilderInitDone;
};

class EaLayer32PcmEnc : public EaLayer32Enc
{
public:
    static const Guid GUID = 'L32P';

    static EncoderDesc* GetEncoderDesc();

private:
    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);

    static EncoderDesc sEncoderDesc;
};

class EaLayer32SpikeEnc : public EaLayer32Enc
{
public:
    static const Guid GUID = 'L32S';

    static EncoderDesc* GetEncoderDesc();

private:
    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);

    static EncoderDesc sEncoderDesc;
};

#endif
