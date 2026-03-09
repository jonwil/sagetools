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

#ifndef ENCPCM16BIG_H
#define ENCPCM16BIG_H

#include "encoder.h"

class Pcm16BigEnc : public Encoder
{
public:
    static const Guid GUID = 'P6B0';

    static EncoderDesc* GetEncoderDesc();
    virtual int Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* bytesEncoded, void* pSeekData, int* pSeekDataBytes);
    virtual int Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);

private:
    friend class EncoderRegistry;
    static EncoderDesc sEncoderDesc;

    static Encoder* CreateInstance(int numChannels, int sampleRate, System* pSystem);
};

#endif
