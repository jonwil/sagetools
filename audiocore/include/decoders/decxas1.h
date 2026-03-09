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

#ifndef DECXAS1_H
#define DECXAS1_H

#include "decoder.h"

class Xas1Dec : public Decoder
{
public:
    static const Guid GUID = 'Xas1';

    static DecoderDesc* GetDecoderDesc();

private:
    static const unsigned char SAMPLES_PER_BLOCK = 32;
    static const unsigned char BYTES_PER_BLOCK = 19;
    static const unsigned char BLOCKS_PER_GROUP = 4;
    static const unsigned char SAMPLES_PER_GROUP = SAMPLES_PER_BLOCK * BLOCKS_PER_GROUP;
    static const unsigned char BYTES_PER_GROUP = BYTES_PER_BLOCK * BLOCKS_PER_GROUP;
    static DecoderDesc sDecoderDesc;
    static const float sShiftMulLut[13];
    static const float sFilterPairs[4][2];

    static unsigned int GetSize(unsigned int, unsigned int* pAlignment)
    {
        *pAlignment = 16;
        return sizeof(Xas1Dec);
    }

    static bool CreateInstanceEvent(Decoder* pDecoder);

public:
    static int DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples);

private:
    void Reset()
    {
        mRemainingSamples = 0;
        mpEncodedSample = 0;
    }

    void DecodeChannel(unsigned char* pSrc, float* pDst);
    unsigned char* mpEncodedSample;
    int mRemainingSamples;
};

#endif
