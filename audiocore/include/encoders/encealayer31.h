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

#ifndef ENCEALAYER31_H
#define ENCEALAYER31_H

#include "encoder.h"
#include "decoder.h"

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
struct EALAYER3STRUCT;
struct MPEGAUDIOHDR;

class EaLayer3Enc : public Encoder
{
public:
    static const Guid GUID = 'EL31';

    static EncoderDesc* GetEncoderDesc();
    virtual int Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int GetSeekMemoryRequired(int numFrames);
    virtual void Release();

private:
    static const int EALAYER3_AVERAGEDATARATE = 384000 / 8;
    static const int EALAYER3_GRANULESAMPLES = 576;
    static const int EALAYER3_LATENCY_FRAMES = 1105;
    static const int EALAYER3_RAW_BLOCKSAMPLES = (EALAYER3_GRANULESAMPLES * 2) - EALAYER3_LATENCY_FRAMES;
    static EncoderDesc sEncoderDesc;

    EALAYER3STRUCT* mpLameStructArray;
    int mNumEncoderInstances;
    int mIsInitialized;
    int   mResidualSamples;
    short** mppSourceData;
    short* mppSourceDataPadding;
    unsigned char* mpGranule2TempStorage;
    int mGranule2TempStorageSize;
    int mGranule2Bytes;
    unsigned char* mpBufferedEncodedData;
    int mBufferedEncodedDataBytes;
    int mGranulesProduced;
    void* mpBufferedSeekData;
    int mBufferedSeekDataBytes;

    void PreEncodeSetup(float* pSrc, int samples);
    int EncodeBlock(unsigned char* pDst, int numSamples, int* bytesEncoded, int flush, void* pSeekData, int* pSeekDataBytes);
    lame_global_flags* InitLAYER3(EALAYER3STRUCT* pLameStruct, int numChannels);
    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);
};

#endif
