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

#ifndef DECEALAYER31_H
#define DECEALAYER31_H

#include "private\mpegcommon.h"
#include "decoder.h"

const int MPEG_AUDIO_LAYER_LATENCY[] = { 0, 481, 481, 1105 };

class EaLayer3DecBase : public Decoder
{
protected:
    static const unsigned short EALAYER3_BLOCKSAMPLES = 576;
    static const char USE_EALAYER31 = 0;
    static const char USE_EALAYER32PCM = 1;
    static const char USE_EALAYER32SPIKE = 2;

    static unsigned int GetSize(unsigned int numChannels, unsigned int* pAlignment);
    static bool CreateInstance(Decoder* pDecoder, char version);
    static void ReleaseEvent(Decoder* pDecoder);

public:
    static int DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples);

private:
    void Reset();
    void SkipBlocks();
    void DecodePcm(float** ppDst, void* pSrc, int numChannels, int numSamples);
    int DecodeSpecialBlock(unsigned char* pSrc, float** ppOutputSamples, EALayer3Core* pEALayer3CoreDecoder);
    int DecodeGranule(unsigned char* pSrc, float** ppOutputSamples, EALayer3Core* pEALayer3CoreDecoder, int* pFramesDecoded, int* pLatencyConsumed, int* pSkipConsumed, int numChannnels);

    EALayer3Core* mpLoadedEALayer3Core;
    unsigned char* mpEncodedSample;
    EALayer3Core** mppEaLayer3Core;
    int mRemainingSamples;
    int mTotalChannels;
    int mNumEaLayer3CoreInstances;
    int mLatency;
    int mSkipSamples;
    char mVersion;
    bool mNewFeed;
};

class EaLayer31Dec : public EaLayer3DecBase
{
public:
    static const Guid GUID = 'EL31';

    static DecoderDesc* GetDecoderDesc();

private:
    static DecoderDesc sDecoderDesc;

    static bool CreateInstanceEvent(Decoder* pDecoder);
};

#endif
