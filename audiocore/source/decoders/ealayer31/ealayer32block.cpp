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

#include "ealayer32block.h"
#include "system.h"
#include "endian.h"
#include <string.h>

int EaLayer32Block::Read(const void* pInputBuffer)
{
    uintptr_t pBlockStart = reinterpret_cast<uintptr_t>(pInputBuffer);
    int headerSize = 0;
    int pcmDataBytes = 0;
    short minimalHeader;
    ENDIAN::PutUB(minimalHeader, *reinterpret_cast<short*>(pBlockStart));
    headerSize += sizeof(minimalHeader);
    int extendedHeaderFlag = (minimalHeader >> 15) & 1;
    mNumChannels = ((minimalHeader >> 14) & 1) + 1;
    int blockSize = minimalHeader & 0x0FFF;

    if (extendedHeaderFlag != 0)
    {
        int extHeader;
        ENDIAN::PutUB(extHeader, *reinterpret_cast<unsigned int*>(pBlockStart + headerSize));
        headerSize += sizeof(extHeader);
        mBlockOffsetMode = static_cast<BlockOffsetMode>((extHeader >> 30) & 0x0003);
        mOffsetSamples = (extHeader >> 20) & 0x03FF;
        mPcmSamples = (extHeader >> 10) & 0x03FF;
        mGranuleSize = extHeader & 0x03FF;
        mGranuleOffset = 0;

        if (0 < mGranuleSize)
        {
            mGranuleOffset = headerSize;
        }

        mPcmDataOffset = headerSize + mGranuleSize;
        pcmDataBytes = mPcmSamples * mNumChannels * static_cast<int>(sizeof(short));
    }
    else
    {
        mGranuleSize = blockSize - headerSize;
        mGranuleOffset = headerSize;
        mPcmDataOffset = 0;
        mPcmSamples = 0;
        mOffsetSamples = 0;
        mBlockOffsetMode = BLOCKOFFSETMODE_IGNORE;
    }

    return blockSize;
}

int EaLayer32Block::Write(void* pOutputBuffer, void* pGranule, int granuleSize, float* pPcmData0, float* pPcmData1, int pcmSamples, int numChannels, BlockOffsetMode blockOffsetMode, int offsetSamples, System* pSystem)
{
    int headerSize = 2;
    int headerFlag = 0;
    int channelFlag = ((numChannels - 1) & 1) << 14;

    if (pcmSamples != 0 || offsetSamples != 0)
    {
        headerSize = 6;
        headerFlag = 0x8000;
    }

    int pcmDataSize = pcmSamples * numChannels * static_cast<int>(sizeof(short));
    int blockSize = headerSize + granuleSize + pcmDataSize;
    unsigned char* pWrite = reinterpret_cast<unsigned char*>(pOutputBuffer);
    short headerField = static_cast<short>(headerFlag | channelFlag | blockSize);
    ENDIAN::PutUB(*reinterpret_cast<short*>(pWrite), headerField);
    pWrite += sizeof(headerField);

    if (headerFlag != 0)
    {
        int extHeader = (blockOffsetMode << 30) | (offsetSamples << 20) | (pcmSamples << 10) | granuleSize;
        ENDIAN::PutUB(*reinterpret_cast<int*>(pWrite), extHeader);
        pWrite += sizeof(extHeader);
    }

    if (0 < granuleSize)
    {
        memcpy(reinterpret_cast<void*>(pWrite), pGranule, static_cast<size_t>(granuleSize));
        pWrite += granuleSize;
    }

    for (int i = 0; i < pcmSamples; i++)
    {
        short value0 = static_cast<short>(pPcmData0[i]);
        short* pDst0 = reinterpret_cast<short*>(pWrite);
        ENDIAN::PutUB(*pDst0, value0);
        pWrite += sizeof(short);

        if (numChannels == 2)
        {
            short value1 = static_cast<short>(pPcmData1[i]);
            short* pDst1 = reinterpret_cast<short*>(pWrite);
            ENDIAN::PutUB(*pDst1, value1);
            pWrite += sizeof(short);
        }
    }

    return blockSize;
}

int EaLayer32Block::GetBlockSize() const
{
    return CalcSize(mGranuleSize, mPcmSamples, mNumChannels, mOffsetSamples);
}

int EaLayer32Block::CalcSize(int granuleSize, int pcmSamples, int numChannels, int offsetSamples)
{
    int headerSize = 2;

    if (pcmSamples != 0 || offsetSamples != 0)
    {
        headerSize = 2 + 4;
    }

    int pcmDataSize = pcmSamples * numChannels * static_cast<int>(sizeof(short));
    int blockSize = headerSize + granuleSize + pcmDataSize;
    return blockSize;
}

int EaLayer32Block::GetUsableSamples() const
{
    int result = MAX_BLOCK_SIZE_SAMPLES;

    switch (mBlockOffsetMode)
    {
    case BLOCKOFFSETMODE_IGNORE:
        result = MAX_BLOCK_SIZE_SAMPLES - mOffsetSamples;

        if (mGranuleOffset == 0)
        {
            result = mPcmSamples;
        }

        break;
    case BLOCKOFFSETMODE_MUTE:
        if (mGranuleOffset == 0)
        {
            result = mPcmSamples + mPcmSamples;
        }

        break;
    case BLOCKOFFSETMODE_PRESERVE:
        if (mGranuleOffset == 0)
        {
            result = mPcmSamples;
        }

        break;
    }

    return result;
}
