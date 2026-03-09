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

#ifndef ENCXAS1_H
#define ENCXAS1_H

#include "encoder.h"

const static unsigned char MAX_ROUTE_CHANNELS = 64;

class Xas1Enc : public Encoder
{
public:
    static const Guid GUID = 'Xas1';

    static EncoderDesc* GetEncoderDesc();
    virtual int Encode(float* pSrc, unsigned char* pDst, int samples, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);

private:
    friend class EncoderRegistry;
    static const unsigned char XAS_MINIBLOCKSAMPLES = 32;
    static const unsigned char XAS_MINIBLOCKGROUP = 4;
    static const unsigned char XAS_BLOCKSAMPLES = XAS_MINIBLOCKSAMPLES * XAS_MINIBLOCKGROUP;
    static const unsigned char XAS_MINIBLOCKBYTES = 19;
    static const unsigned char XAS_MINIBLOCKDATABYTES = 15;
    static const unsigned char XAS_BLOCKBYTES = XAS_MINIBLOCKBYTES * XAS_MINIBLOCKGROUP;
    static EncoderDesc sEncoderDesc;
    static const float sFilterPairs[4][2];

    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);
    void EncodeBlock(float* src, unsigned char* dest, int channels);
    void CustomInterleaveXAS(unsigned char* pSrcBlock, unsigned char* pDstBlock);

    int   mResidualSamples;
    float mResidueBuffer[MAX_ROUTE_CHANNELS * XAS_BLOCKSAMPLES];
};

#endif
