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

#ifndef DECPCM16BIG_H
#define DECPCM16BIG_H

#include "decoder.h"

class Pcm16BigDec : public Decoder
{
public:
    static const Guid GUID = 'P6B0';

    static DecoderDesc* GetDecoderDesc();
    static int DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples);

private:
    static DecoderDesc sDecoderDesc;

    static unsigned int GetSize(unsigned int, unsigned int* pAlignment)
    {
        *pAlignment = 16;
        return sizeof(Pcm16BigDec);
    }

    static bool CreateInstanceEvent(Decoder* pDecoder);
    void Reset();

    uintptr_t mpSrc;
    int mRemainingSamples;
};

#endif
