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

#ifndef ENCLAYER3_H
#define ENCLAYER3_H

#include "encoder.h"

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;

struct LAYER3STRUCT;
struct MPEGAUDIOHDR;

class Layer3Enc : public Encoder
{
public:
    static const Guid GUID = 'MP30';

    virtual int Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* bytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    static EncoderDesc* GetEncoderDesc();
    virtual void Release();

private:
    friend class EncoderRegistry;
    static const int LAYER3_AVERAGEDATARATE = 384000 / 8;
    static EncoderDesc sEncoderDesc;

    LAYER3STRUCT* mpLameStructArray;
    int mNumEncoderInstances;
    int mIsInitialized;
    int mResidualSamples;
    short** mppSourceData;
    short* mppSourceDataPadding;

    void PreEncodeSetup(float* pSrc, int samples);
    int EncodeBlock(unsigned char* pDst, int numSamples, int* bytesEncoded, int flush);
    int ChooseMpegBitRate(int tempBR, int samplerate);
    lame_global_flags* InitLAYER3(LAYER3STRUCT* pLameStruct, int numChannels);
    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);
};

#endif
