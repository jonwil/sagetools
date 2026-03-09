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

#ifndef EALAYER32BLOCK_H
#define EALAYER32BLOCK_H

#include "base.h"

class System;

enum BlockOffsetMode
{
    BLOCKOFFSETMODE_IGNORE = 0,
    BLOCKOFFSETMODE_PRESERVE = 1,
    BLOCKOFFSETMODE_MUTE = 2,
    BLOCKOFFSETMODE_MAX = 3,
};

class EaLayer32Block
{
public:
    static const int MAX_BLOCK_SIZE_SAMPLES = 576;
    static int ReadBlockSize(void* pBlock);
    static int CalcSize(int granuleSize, int pcmSamples, int numChannels, int offsetSamples);
    static int Write(void* pOutputBuffer, void* pGranule, int granuleSize, float* pPcmData0, float* pPcmData1, int pcmSamples, int numChannels, BlockOffsetMode blockOffsetMode, int offsetSamples, System* pSystem);
    void Init(System* pSystem);
    int GetBlockSize() const;
    int GetUsableSamples() const;
    int GetPcmSamples() const { return mPcmSamples; }
    int GetPcmDataOffset() const { return mPcmDataOffset; }
    int GetOffsetSamples() const { return mOffsetSamples; }
    BlockOffsetMode GetBlockOffsetMode() { return mBlockOffsetMode; }
    int GetGranuleOffset() const { return mGranuleOffset; };
    int Read(const void* pInputBuffer);
private:
    System* mpSystem;
    int mGranuleOffset;
    int mGranuleSize;
    int mPcmDataOffset;
    int mPcmSamples;
    int mNumChannels;
    int mOffsetSamples;
    BlockOffsetMode mBlockOffsetMode;
};

inline void EaLayer32Block::Init(System* pSystem)
{
}

inline int EaLayer32Block::ReadBlockSize(void* pBlock)
{
    unsigned char* pData = static_cast<unsigned char*>(pBlock);
    return (pData[0] << 8 | pData[1]) & 0x0FFF;
}

#endif
