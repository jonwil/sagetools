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

#ifndef DECLAYER3_H
#define DECLAYER3_H

#include "base.h"
#include "decoder.h"
#include "private\mpegcommon.h"

class Layer3Dec : public Decoder, CMpegLayer3Base
{
public:
    static const Guid GUID = 'MP30';

    Layer3Dec() {};
    Layer3Dec(int numChannels);
    virtual ~Layer3Dec() {};
    int Open(void* buf, int filesize);
    static DecoderDesc* GetDecoderDesc();

private:
    static const unsigned short LAYER3_BLOCKSAMPLES = 1152;
    static DecoderDesc sDecoderDesc;

    static unsigned int GetSize(unsigned int numChannels, unsigned int* pAlignment)
    {
        *pAlignment = 16;
        return sizeof(Layer3Dec);
    }

    static bool CreateInstanceEvent(Decoder* pDecoder);
    static void ReleaseEvent(Decoder* pDecoder);

public:
    static int DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples);

private:
    int Decode(float* pOutputSamples);
    int OpenLayer();
    bool GetSideInfo();
    void GetScaleFactors(unsigned int ch, unsigned int gr);
    void GetLsfScaleData(int ch, int gr, unsigned char scaleFacBuf[54]);
    void GetLsfScaleFactors(int ch, int gr);
    void DecodeHuffman(int ch, int gr, float output[32 * 18], int part2_start);

    __declspec(align(16)) float mFrameBuf[2 * LAYER3_BLOCKSAMPLES];
    Bit_Reserve br;
    unsigned char* mpEncodedSample;
    int mRemainingSamples;
    unsigned char mReset;
    static float sLayer3SharedMemory[];
};

#endif
