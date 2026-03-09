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

#ifndef ENCODERHELPER_H
#define ENCODERHELPER_H

#include "encoder.h"
#include "encoderregistry.h"

class EncoderHelper
{
public:
    static EncoderHelper* CreateInstance(void* psound, int guid);
    float GetAverageDataRate();
    bool IsSeekable();
    int GetSeekDataVersion() { return mpEncoder->GetSeekDataVersion(); }
    unsigned short GetMaxLatency() { return mpEncoder->GetMaxLatency(); }
    int Encode(short* pSampleData[], unsigned char** pEncodedData, int numSamples, int* bytesEncoded, void* psound, unsigned char** ppSeekData, int* pSeekDataBytes);
    int Flush(unsigned char** pEncodedData, int* bytesEncoded, unsigned char** ppSeekData, int* pSeekDataBytes);
    void Release();

private:
    EncoderRegistry* mpEncoderRegistry;
    EncoderExtended* mpEncoder;
    unsigned char* mpEncodedDataBuffer;
    unsigned char* mpEncodedSeekBuffer;
    unsigned char* mpFlushedData;
    unsigned char* mpFlushedSeek;
    int mEncodedDataBufferSize;
    int mEncodedSeekBufferSize;
    bool mCanFlush;
};

#endif
