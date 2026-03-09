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

#include "system.h"
#include "private\mpegcommon.h"
#include "ibase.h"

EALayer3Core::EALayer3Core(int numChannels)
{
    int result = AllocateSynth(numChannels);
    result = AllocateHybrid(numChannels);
}

EALayer3Core::~EALayer3Core()
{
}

__declspec(align(16)) typedef struct EALayer3Temp
{
    int granule[3][CMpegLayer3Base::SAMPLES_PER_GRANULE];
} EALayer3Temp;

short EALayer3Core::GetBitsSafely(int n)
{
    if (n == 0)
    {
        return 0;
    }
    else
    {
        return GetBits(n);
    }
}

unsigned EALayer3Core::GetBitPos()
{
    return static_cast<unsigned int>(8 * (reinterpret_cast<uintptr_t>(mBufPtr) - reinterpret_cast<uintptr_t>(mBufPtrBase)) - (unsigned)mShiftRegBits);
}

void EALayer3Core::RewindBits(int n)
{
    mShiftRegBits += n;
    mBufPtr -= ((unsigned)mShiftRegBits >> 3);
    mShiftRegBits &= 7;

    if (mShiftRegBits)
    {
        mShiftReg = static_cast<unsigned int>(mBufPtr[-1] << (32 - mShiftRegBits));
    }
}

bool EALayer3Core::GetSideInfo(unsigned int gr)
{
    int ch;

    if (!mLSF)
    {
        if (gr == 1)
        {
            LoadBitRegister();

            for (ch = 0; ch < mChannels; ch++)
            {
                unsigned char* pScfsi = mpTemp->sideInfo.ch[ch].scfsi;

                pScfsi[0] = static_cast<unsigned char>(mShiftReg >> (32 - 1));
                pScfsi[1] = static_cast<unsigned char>((mShiftReg >> (32 - (1 + 1))) & 1);
                pScfsi[2] = static_cast<unsigned char>((mShiftReg >> (32 - (1 + 1 + 1))) & 1);
                pScfsi[3] = static_cast<unsigned char>((mShiftReg >> (32 - (1 + 1 + 1 + 1))) & 1);
                mShiftReg <<= 4;
                mShiftRegBits -= 4;
            }
        }

        for (ch = 0; ch < mChannels; ch++)
        {
            GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
            pGranuleInfo->part2And3Length = static_cast<unsigned short>(GetBits(12));
            LoadBitRegister();
            pGranuleInfo->bigValues = static_cast<unsigned short>(mShiftReg >> (32 - 9));
            pGranuleInfo->globalGain = static_cast<unsigned char>((mShiftReg >> (32 - (9 + 8))) & 0xff);
            pGranuleInfo->scaleFacCompress = static_cast<unsigned char>((mShiftReg >> (32 - (9 + 8 + 4))) & 0xf);
            pGranuleInfo->windowSwitchingFlag = static_cast<unsigned char>((mShiftReg >> (32 - (9 + 8 + 4 + 1))) & 1);
            mShiftReg <<= (9 + 8 + 4 + 1);
            mShiftRegBits -= (9 + 8 + 4 + 1);
            LoadBitRegister();

            if (pGranuleInfo->windowSwitchingFlag)
            {
                pGranuleInfo->blockType = static_cast<unsigned char>(mShiftReg >> (32 - 2));
                pGranuleInfo->mixedBlockFlag = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1))) & 0x1);
                pGranuleInfo->tableSelect[0] = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1 + 5))) & 0x1f);
                pGranuleInfo->tableSelect[1] = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1 + 5 + 5))) & 0x1f);
                pGranuleInfo->subBlockGain[0] = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1 + 5 + 5 + 3))) & 0x7);
                pGranuleInfo->subBlockGain[1] = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1 + 5 + 5 + 3 + 3))) & 0x7);
                pGranuleInfo->subBlockGain[2] = static_cast<unsigned char>((mShiftReg >> (32 - (2 + 1 + 5 + 5 + 3 + 3 + 3))) & 0x7);

                if (pGranuleInfo->blockType == 0)
                {
                    return false;
                }
                else if (pGranuleInfo->blockType == 2 && pGranuleInfo->mixedBlockFlag == 0)
                {
                    pGranuleInfo->region0Count = 8;
                }
                else
                {
                    pGranuleInfo->region0Count = 7;
                }

                pGranuleInfo->region1Count = static_cast<unsigned char>(20u - pGranuleInfo->region0Count);
            }
            else
            {
                pGranuleInfo->tableSelect[0] = static_cast<unsigned char>(mShiftReg >> (32 - 5));
                pGranuleInfo->tableSelect[1] = static_cast<unsigned char>((mShiftReg >> (32 - (5 + 5))) & 0x1f);
                pGranuleInfo->tableSelect[2] = static_cast<unsigned char>((mShiftReg >> (32 - (5 + 5 + 5))) & 0x1f);
                pGranuleInfo->region0Count = static_cast<unsigned char>((mShiftReg >> (32 - (5 + 5 + 5 + 4))) & 0xf);
                pGranuleInfo->region1Count = static_cast<unsigned char>((mShiftReg >> (32 - (5 + 5 + 5 + 4 + 3))) & 0x7);
                pGranuleInfo->blockType = 0;
            }

            pGranuleInfo->preFlag = static_cast<unsigned char>((mShiftReg >> (32 - (22 + 1))) & 0x1);
            pGranuleInfo->scaleFacScale = (mShiftReg >> (32 - (22 + 1 + 1))) & 0x1;
            pGranuleInfo->count1TableSelect = static_cast<unsigned char>((mShiftReg >> (32 - (22 + 1 + 1 + 1))) & 0x1);
            mShiftReg <<= (22 + 1 + 1 + 1);
            mShiftRegBits -= (22 + 1 + 1 + 1);
        }
    }
    else
    {
        for (ch = 0; ch < mChannels; ch++)
        {
            GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][0];
            pGranuleInfo->part2And3Length = static_cast<unsigned short>(GetBits(12));
            pGranuleInfo->bigValues = static_cast<unsigned short>(GetBits(9));
            pGranuleInfo->globalGain = static_cast<unsigned char>(GetBits(8));
            pGranuleInfo->scaleFacCompress = static_cast<unsigned short>(GetBits(9));
            pGranuleInfo->windowSwitchingFlag = static_cast<unsigned char>(GetBits(1));

            if (pGranuleInfo->windowSwitchingFlag)
            {
                pGranuleInfo->blockType = static_cast<unsigned char>(GetBits(2));
                pGranuleInfo->mixedBlockFlag = static_cast<unsigned char>(GetBits(1));
                pGranuleInfo->tableSelect[0] = static_cast<unsigned char>(GetBits(5));
                pGranuleInfo->tableSelect[1] = static_cast<unsigned char>(GetBits(5));
                pGranuleInfo->subBlockGain[0] = static_cast<unsigned char>(GetBits(3));
                pGranuleInfo->subBlockGain[1] = static_cast<unsigned char>(GetBits(3));
                pGranuleInfo->subBlockGain[2] = static_cast<unsigned char>(GetBits(3));

                if (pGranuleInfo->blockType == 0)
                {
                    return false;
                }
                else if (pGranuleInfo->blockType == BLOCKTYPE_SHORT && pGranuleInfo->mixedBlockFlag == 0)
                {
                    pGranuleInfo->region0Count = 8;
                }
                else
                {
                    pGranuleInfo->region0Count = 7;
                    pGranuleInfo->region1Count = static_cast<unsigned char>(20u - pGranuleInfo->region0Count);
                }
            }
            else
            {
                pGranuleInfo->tableSelect[0] = static_cast<unsigned char>(GetBits(5));
                pGranuleInfo->tableSelect[1] = static_cast<unsigned char>(GetBits(5));
                pGranuleInfo->tableSelect[2] = static_cast<unsigned char>(GetBits(5));
                pGranuleInfo->region0Count = static_cast<unsigned char>(GetBits(4));
                pGranuleInfo->region1Count = static_cast<unsigned char>(GetBits(3));
                pGranuleInfo->blockType = 0;
            }

            pGranuleInfo->scaleFacScale = GetBits(1);
            pGranuleInfo->count1TableSelect = static_cast<unsigned char>(GetBits(1));
        }
    }

    return true;
}

void EALayer3Core::GetScaleFactors(int ch, int gr)
{
    int sfb;
    int window;
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    int length0 = slen[0][pGranuleInfo->scaleFacCompress];
    int length1 = slen[1][pGranuleInfo->scaleFacCompress];
    Layer3ScaleFactors* pScaleFactors = &mScaleFactors[ch];

    if (pGranuleInfo->windowSwitchingFlag && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
    {
        if (pGranuleInfo->mixedBlockFlag)
        {
            for (sfb = 0; sfb < 8; sfb++)
            {
                pScaleFactors->longBlock[sfb] = GetBitsSafely(length0);
            }

            for (sfb = 3; sfb < 6; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = GetBitsSafely(length0);
                }
            }

            for (sfb = 6; sfb < 12; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = GetBitsSafely(length1);
                }
            }

            pScaleFactors->shortBlock[0][12] = 0;
            pScaleFactors->shortBlock[1][12] = 0;
            pScaleFactors->shortBlock[2][12] = 0;
        }
        else
        {
            pScaleFactors->shortBlock[0][0] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][0] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][0] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][1] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][1] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][1] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][2] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][2] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][2] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][3] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][3] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][3] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][4] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][4] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][4] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][5] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[1][5] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[2][5] = GetBitsSafely(length0);
            pScaleFactors->shortBlock[0][6] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][6] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][6] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][7] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][7] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][7] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][8] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][8] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][8] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][9] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][9] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][9] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][10] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][10] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][10] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][11] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[1][11] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[2][11] = GetBitsSafely(length1);
            pScaleFactors->shortBlock[0][12] = 0;
            pScaleFactors->shortBlock[1][12] = 0;
            pScaleFactors->shortBlock[2][12] = 0;
        }
    }
    else
    {
        unsigned char* pScfsi = mpTemp->sideInfo.ch[ch].scfsi;

        if (!pScfsi[0] || !gr)
        {
            pScaleFactors->longBlock[0] = GetBitsSafely(length0);
            pScaleFactors->longBlock[1] = GetBitsSafely(length0);
            pScaleFactors->longBlock[2] = GetBitsSafely(length0);
            pScaleFactors->longBlock[3] = GetBitsSafely(length0);
            pScaleFactors->longBlock[4] = GetBitsSafely(length0);
            pScaleFactors->longBlock[5] = GetBitsSafely(length0);
        }
        if (!pScfsi[1] || !gr)
        {
            pScaleFactors->longBlock[6] = GetBitsSafely(length0);
            pScaleFactors->longBlock[7] = GetBitsSafely(length0);
            pScaleFactors->longBlock[8] = GetBitsSafely(length0);
            pScaleFactors->longBlock[9] = GetBitsSafely(length0);
            pScaleFactors->longBlock[10] = GetBitsSafely(length0);
        }
        if (!pScfsi[2] || !gr)
        {
            pScaleFactors->longBlock[11] = GetBitsSafely(length1);
            pScaleFactors->longBlock[12] = GetBitsSafely(length1);
            pScaleFactors->longBlock[13] = GetBitsSafely(length1);
            pScaleFactors->longBlock[14] = GetBitsSafely(length1);
            pScaleFactors->longBlock[15] = GetBitsSafely(length1);
        }
        if (!pScfsi[3] || !gr)
        {
            pScaleFactors->longBlock[16] = GetBitsSafely(length1);
            pScaleFactors->longBlock[17] = GetBitsSafely(length1);
            pScaleFactors->longBlock[18] = GetBitsSafely(length1);
            pScaleFactors->longBlock[19] = GetBitsSafely(length1);
            pScaleFactors->longBlock[20] = GetBitsSafely(length1);
        }
        pScaleFactors->longBlock[21] = 0;
        pScaleFactors->longBlock[22] = 0;
    }
}

void EALayer3Core::GetLsfScaleData(int ch, int gr, unsigned char scaleFacBuf[54])
{
    unsigned int new_slen[4] = { 0 };
    unsigned int scalefac_comp;
    unsigned int int_scalefac_comp;
    unsigned int mode_ext = mModeExt;
    int blockTypeNumber;
    int blockNumber = 0;
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    scalefac_comp = pGranuleInfo->scaleFacCompress;

    if (pGranuleInfo->blockType == BLOCKTYPE_SHORT)
    {
        if (pGranuleInfo->mixedBlockFlag == 0)
        {
            blockTypeNumber = 1;
        }
        else if (pGranuleInfo->mixedBlockFlag == 1)
        {
            blockTypeNumber = 2;
        }
        else
        {
            blockTypeNumber = 0;
        }
    }
    else
    {
        blockTypeNumber = 0;
    }

    if (!(((mode_ext == 1) || (mode_ext == 3)) && (ch == 1)))
    {
        if (scalefac_comp < 400)
        {
            new_slen[0] = (scalefac_comp >> 4) / 5;
            new_slen[1] = (scalefac_comp >> 4) % 5;
            new_slen[2] = (scalefac_comp & 0xF) >> 2;
            new_slen[3] = (scalefac_comp & 3);
            pGranuleInfo->preFlag = 0;
            blockNumber = 0;
        }
        else if (scalefac_comp < 500)
        {
            new_slen[0] = ((scalefac_comp - 400) >> 2) / 5;
            new_slen[1] = ((scalefac_comp - 400) >> 2) % 5;
            new_slen[2] = (scalefac_comp - 400) & 3;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blockNumber = 1;
        }
        else if (scalefac_comp < 512)
        {
            new_slen[0] = (scalefac_comp - 500) / 3;
            new_slen[1] = (scalefac_comp - 500) % 3;
            new_slen[2] = 0;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 1;
            blockNumber = 2;
        }
    }

    if ((((mode_ext == 1) || (mode_ext == 3)) && (ch == 1)))
    {
        int_scalefac_comp = scalefac_comp >> 1;

        if (int_scalefac_comp < 180)
        {
            new_slen[0] = int_scalefac_comp / 36;
            new_slen[1] = (int_scalefac_comp % 36) / 6;
            new_slen[2] = (int_scalefac_comp % 36) % 6;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blockNumber = 3;
        }
        else if (int_scalefac_comp < 244)
        {
            new_slen[0] = ((int_scalefac_comp - 180) & 0x3F) >> 4;
            new_slen[1] = ((int_scalefac_comp - 180) & 0xF) >> 2;
            new_slen[2] = (int_scalefac_comp - 180) & 3;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blockNumber = 4;
        }
        else if (int_scalefac_comp < 255)
        {
            new_slen[0] = (int_scalefac_comp - 244) / 3;
            new_slen[1] = (int_scalefac_comp - 244) % 3;
            new_slen[2] = 0;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blockNumber = 5;
        }
    }

    for (unsigned int x = 0; x < 45; x++)
    {
        scaleFacBuf[x] = 0;
    }

    int m = 0;

    for (unsigned int i = 0; i < 4; i++)
    {
        for (unsigned int j = 0; j < sNumSfbBlock[blockNumber][blockTypeNumber][i]; j++)
        {
            scaleFacBuf[m] = static_cast<unsigned char>(GetBitsSafely(new_slen[i]));
            m++;
        }
    }
}

void EALayer3Core::GetLsfScaleFactors(int ch, int gr)
{
    unsigned int m = 0;
    unsigned int sfb;
    unsigned int window;
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    Layer3ScaleFactors* pScaleFactors = &mScaleFactors[ch];
    unsigned char scaleFacBuf[54];
    GetLsfScaleData(ch, gr, scaleFacBuf);

    if (pGranuleInfo->windowSwitchingFlag && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
    {
        if (pGranuleInfo->mixedBlockFlag)
        {
            for (sfb = 0; sfb < 8; sfb++)
            {
                pScaleFactors->longBlock[sfb] = scaleFacBuf[m];
                m++;
            }

            for (sfb = 3; sfb < 12; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = scaleFacBuf[m];
                    m++;
                }
            }

            for (window = 0; window < 3; window++)
                pScaleFactors->shortBlock[window][12] = 0;
        }
        else
        {
            for (sfb = 0; sfb < 12; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = scaleFacBuf[m];
                    m++;
                }
            }

            pScaleFactors->shortBlock[0][12] = 0;
            pScaleFactors->shortBlock[1][12] = 0;
            pScaleFactors->shortBlock[2][12] = 0;
        }
    }
    else
    {
        for (sfb = 0; sfb < 21; sfb++)
        {
            pScaleFactors->longBlock[sfb] = scaleFacBuf[m];
            m++;
        }

        pScaleFactors->longBlock[21] = 0;
        pScaleFactors->longBlock[22] = 0;
    }
}

void EALayer3Core::DecodeHuffman(int ch, int gr, float output[32 * 18], int part2_start)
{
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    int x;
    int y;
    int part2_3_end = part2_start + pGranuleInfo->part2And3Length;
    int num_bits;
    int region1Start;
    int region2Start;
    const short* pTable;
    short index;
    int SToPowerOf4over3Count = 0;
    const int MAXINDEX = 32;
    short SToPowerOf4over3Index[MAXINDEX];
    __declspec(align(16)) short SToPowerOf4over3Input[MAXINDEX];
    __declspec(align(16)) float SToPowerOf4over3Results[MAXINDEX];

    if ((pGranuleInfo->windowSwitchingFlag) && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
    {
        region1Start = 36;
        region2Start = 576;
    }
    else
    {
        region1Start = sfBandIndex[sfreq].l[pGranuleInfo->region0Count + 1];
        region2Start = sfBandIndex[sfreq].l[pGranuleInfo->region0Count + pGranuleInfo->region1Count + 2];
    }

    index = 0;
    int bigValues = pGranuleInfo->bigValues << 1;

    const int regionEnd[] =
    {
        Min(region1Start, bigValues),
        Min(region2Start, bigValues),
        bigValues
    };

    float globalGain = mpLoadedTwoToNegativeQuarterPower[(pGranuleInfo->globalGain - 255) * -1];
    float negativeGlobalGain = -globalGain;

    for (int region = 0; region < 3; region++)
    {
        pTable = mpLoadedTableSelect[region];
        const int tableSelect = pGranuleInfo->tableSelect[region];
        const int linearBits = sHuffTableLinearBits[tableSelect];

        if (pTable)
        {
            for (; index < regionEnd[region]; index += 2)
            {
                num_bits = GetBitPos();

                while ((mShiftRegBits <= 24) && ((num_bits + mShiftRegBits) < part2_3_end))
                {
                    mShiftReg |= *mBufPtr++ << (24 - mShiftRegBits);
                    mShiftRegBits += 8;
                }

                const unsigned int byteIndex = mShiftReg >> 24;
                const short linearEntry = pTable[byteIndex];

                if (linearEntry >= 0)
                {
                    const short symbolSize = static_cast<short>(linearEntry >> 8);
                    y = linearEntry & 0xff;
                    mShiftRegBits -= symbolSize;
                    mShiftReg <<= symbolSize;
                }
                else
                {
                    mShiftRegBits -= 8;
                    mShiftReg <<= 8;
                    const short* pEntry = &pTable[-linearEntry];

                    while ((y = *pEntry++) < 0)
                    {
                        if ((int)mShiftReg < 0)
                        {
                            pEntry -= y;
                        }

                        mShiftRegBits--;
                        mShiftReg <<= 1;
                    }
                }

                x = y >> 4;
                y &= 0xf;

                if (x == 15 && linearBits)
                {
                    x += (mShiftReg >> (32 - linearBits));
                    mShiftReg <<= linearBits;
                    mShiftRegBits -= linearBits;
                }

                if (x)
                {
                    if ((int)mShiftReg < 0)
                    {
                        output[index] = negativeGlobalGain;
                    }
                    else
                    {
                        output[index] = globalGain;
                    }

                    if (x < 32)
                    {
                        output[index] *= sToPowerOf4over3Coefficients0To31[x];
                    }
                    else
                    {
                        SToPowerOf4over3Index[SToPowerOf4over3Count] = static_cast<unsigned short>(index);
                        SToPowerOf4over3Input[SToPowerOf4over3Count] = static_cast<unsigned short>(x);
                        SToPowerOf4over3Count++;

                        if (SToPowerOf4over3Count >= MAXINDEX)
                        {
                            SToPowerOf4over3(SToPowerOf4over3Count, SToPowerOf4over3Input, SToPowerOf4over3Results);

                            for (int k = 0; k < SToPowerOf4over3Count; k++)
                            {
                                output[SToPowerOf4over3Index[k]] *= SToPowerOf4over3Results[k];
                            }

                            SToPowerOf4over3Count = 0;
                        }
                    }

                    mShiftRegBits--;
                    mShiftReg <<= 1;
                }
                else
                {
                    output[index] = 0.0f;
                }

                while ((mShiftRegBits <= 24) && ((num_bits + mShiftRegBits) < part2_3_end))
                {
                    mShiftReg |= *mBufPtr++ << (24 - mShiftRegBits);
                    mShiftRegBits += 8;
                }

                if (y == 15 && linearBits)
                {
                    y += (mShiftReg >> (32 - linearBits));
                    mShiftReg <<= linearBits;
                    mShiftRegBits -= linearBits;
                }

                if (y)
                {
                    if ((int)mShiftReg < 0)
                    {
                        output[index + 1] = negativeGlobalGain;
                    }
                    else
                    {
                        output[index + 1] = globalGain;
                    }

                    if (y < 32)
                    {
                        output[index + 1] *= sToPowerOf4over3Coefficients0To31[y];
                    }
                    else
                    {
                        SToPowerOf4over3Index[SToPowerOf4over3Count] = static_cast<unsigned short>(index + 1);
                        SToPowerOf4over3Input[SToPowerOf4over3Count] = static_cast<unsigned short>(y);
                        SToPowerOf4over3Count++;

                        if (SToPowerOf4over3Count >= MAXINDEX)
                        {
                            SToPowerOf4over3(SToPowerOf4over3Count, SToPowerOf4over3Input, SToPowerOf4over3Results);

                            for (int k = 0; k < SToPowerOf4over3Count; k++)
                            {
                                output[SToPowerOf4over3Index[k]] *= SToPowerOf4over3Results[k];
                            }

                            SToPowerOf4over3Count = 0;
                        }
                    }

                    mShiftRegBits--;
                    mShiftReg <<= 1;
                }
                else
                {
                    output[index + 1] = 0.0f;
                }
            }
        }
        else
        {
            memset(&output[index], 0, (regionEnd[region] - index) * sizeof(output[0]));
            index = static_cast<short>(regionEnd[region]);
        }
    }

    SToPowerOf4over3(SToPowerOf4over3Count, SToPowerOf4over3Input, SToPowerOf4over3Results);

    for (int k = 0; k < SToPowerOf4over3Count; k++)
    {
        output[SToPowerOf4over3Index[k]] *= SToPowerOf4over3Results[k];
    }

    const HuffCountTable* pCountTable = &sHuffCountTables[pGranuleInfo->count1TableSelect];
    num_bits = GetBitPos();

    while ((num_bits < part2_3_end) && (index < 576))
    {
        while ((mShiftRegBits <= 24) && ((num_bits + mShiftRegBits) < part2_3_end))
        {
            mShiftReg |= *mBufPtr++ << (24 - mShiftRegBits);
            mShiftRegBits += 8;
        }

        const HuffEntry* pEntry = &pCountTable->pEntries[mShiftReg >> pCountTable->maxCodeShifter];
        mShiftRegBits -= pEntry->length;
        mShiftReg <<= pEntry->length;

        if (pEntry->value & 8)
        {
            if ((int)mShiftReg < 0)
            {
                output[index] = negativeGlobalGain;
            }
            else
            {
                output[index] = globalGain;
            }

            mShiftReg <<= 1;
            mShiftRegBits--;
        }
        else
        {
            output[index] = 0.0f;
        }

        if (pEntry->value & 4)
        {
            if ((int)mShiftReg < 0)
            {
                output[index + 1] = negativeGlobalGain;
            }
            else
            {
                output[index + 1] = globalGain;
            }

            mShiftReg <<= 1;
            mShiftRegBits--;
        }
        else
        {
            output[index + 1] = 0.0f;
        }

        if (pEntry->value & 2)
        {
            if ((int)mShiftReg < 0)
            {
                output[index + 2] = negativeGlobalGain;
            }
            else
            {
                output[index + 2] = globalGain;
            }

            mShiftReg <<= 1;
            mShiftRegBits--;
        }
        else
        {
            output[index + 2] = 0.0f;
        }

        if (pEntry->value & 1)
        {
            if ((int)mShiftReg < 0)
            {
                output[index + 3] = negativeGlobalGain;
            }
            else
            {
                output[index + 3] = globalGain;
            }

            mShiftReg <<= 1;
            mShiftRegBits--;
        }
        else
        {
            output[index + 3] = 0.0f;
        }

        index += 4;
        num_bits = GetBitPos();
    }

    if (num_bits > part2_3_end)
    {
        RewindBits(num_bits - part2_3_end);
        index -= 4;
    }

    num_bits = GetBitPos();

    if (num_bits < part2_3_end)
    {
        GetBits(part2_3_end - num_bits);
    }

    if (index < 576)
    {
        memset(&output[index], 0, (576 - index) * sizeof(output[0]));
    }
}

int EALayer3Core::ProcessEALayer3Header(unsigned int hdr)
{
    unsigned idbits, sampratebits;
    idbits = (hdr >> 6) & 3;
    sampratebits = (hdr >> 4) & 3;
    mMode = static_cast<unsigned char>((hdr >> 2) & 3);
    mModeExt = static_cast<unsigned char>(hdr & 3);
    mVersion = static_cast<unsigned char>(idbits & 1);
    mLayer = 3;
    mLSF = static_cast<unsigned char>((idbits == 3) ? 0 : 1);
    mMPEG25 = (idbits == 0 ? 1U : 0U);

    if (mMPEG25)
    {
        mSampFreqIdx = static_cast<unsigned char>(sampratebits + 6);
    }
    else
    {
        mSampFreqIdx = static_cast<unsigned char>(sampratebits + 3 * mLSF);
        sfreq = sampratebits + (mLSF != 0 ? 0 : 3);
    }

    mChannels = (mMode != 3) + 1;
    mSampFreq = sSampleRateTable[mSampFreqIdx];
    Real_mSampFreq = mSampFreq;
    return 0;
}

int EALayer3Core::Open(void* buf, int filesize)
{
    if (mOpened)
    {
        Close();
    }

    mOpened = 1;
    mBufPtr = static_cast<unsigned char*>(buf);
    mBufPtrBase = mBufPtr;
    ProcessHeader(*mBufPtr);
    mResetSynthRequired = 1;
    mFrameSamples = 576;
    mBandOffset[0] = 1;
    mBandOffset[1] = 1;
    Reset();
    return 0;
}

int EALayer3Core::Decode(float** ppOutputSamples)
{
    mBufPtrBase = mBufPtr;
    ProcessEALayer3Header(static_cast<unsigned int>(GetBits(8)));
    Layer3Temp layer3Temp;
    mpTemp = &layer3Temp;
    void* pTempGranules[3];
    __declspec(align(16)) EALayer3Temp eALayer3Temp;
    pTempGranules[0] = (void*)eALayer3Temp.granule[0];
    pTempGranules[1] = (void*)eALayer3Temp.granule[1];
    pTempGranules[2] = (void*)eALayer3Temp.granule[2];
    float* pOutputSamplesLocal[2];
    pOutputSamplesLocal[0] = ppOutputSamples[0];
    pOutputSamplesLocal[1] = ppOutputSamples[1];
    int sharedMemoryBytesInUse = 0;
    unsigned int gr;
    unsigned int ch;
    int i;
    gr = GetBits(1);
    GetSideInfo(gr);
    mpLoadedTwoToNegativeQuarterPower = mpTwoToNegativeQuarterPower;

    for (ch = 0; ch < mChannels; ch++)
    {
        int part2_start = GetBitPos();

        if (mVersion == 1)
        {
            GetScaleFactors(ch, gr);
        }
        else
        {
            GetLsfScaleFactors(ch, gr);
        }

        const GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];

        int numHuffmanTables;

        if (pGranuleInfo->windowSwitchingFlag)
        {
            numHuffmanTables = 2;
        }
        else
        {
            numHuffmanTables = 3;
        }

        for (int i = 0; i < numHuffmanTables; i++)
        {
            mpLoadedTableSelect[i] = mHuffTables[pGranuleInfo->tableSelect[i]].pEntries;
        }

        sHuffCountTables[0].pEntries = gHuffTableCount0;
        sHuffCountTables[0].maxCodeBits = 6;
        sHuffCountTables[0].maxCodeShifter = 26;
        sHuffCountTables[1].pEntries = gHuffTableCount1;
        sHuffCountTables[1].maxCodeBits = 4;
        sHuffCountTables[1].maxCodeShifter = 28;
        DecodeHuffman(ch, gr, static_cast<float*>(pTempGranules[ch]), part2_start);
        Dequantize(ch, gr, static_cast<float*>(pTempGranules[ch]));
    }

    if (mChannels > 1)
    {
        unsigned int is_pos[576];
        float is_rat_io[576];
        K k;
        mpIs_pos = is_pos;
        mpIs_rat_io = is_rat_io;
        mpK = &k;
        Stereo(gr, reinterpret_cast<float (*)[32][18]>(pTempGranules[0]));
    }

    sharedMemoryBytesInUse = 0;

    for (ch = 0; ch < mChannels; ch++)
    {
        const GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];

        if (pGranuleInfo->windowSwitchingFlag && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
        {
            Reorder(ch, gr, static_cast<float*>(pTempGranules[ch]), static_cast<float*>(pTempGranules[2]));
        }
        else
        {
            void* pTemp = pTempGranules[ch];
            pTempGranules[ch] = pTempGranules[2];
            pTempGranules[2] = pTemp;
        }

        AntiAlias(ch, gr, static_cast<float*>(pTempGranules[2]));
        ReorderForVectoring(static_cast<float*>(pTempGranules[2]), reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]));
        mpLoadedPrevBlockX4 = mpPrevBlockX4;
        Hybrid(ch, gr, reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]));
        mpLoadedPolySynthHistoryF = mpPolySynthHistoryF;

        if (mResetSynthRequired)
        {
            ResetSynth();
            mResetSynthRequired = 0;
        }

        if (staticDetectCPU.IsSSE())
        {
            FrequencyInversionX4SSE(reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]));
            ReorderForFPolySynthSSE(reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]), static_cast<float (*)[32]>(pTempGranules[2]));
        }
        else
        {
            FrequencyInversionX4(reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]));
            ReorderForFPolySynth(reinterpret_cast<float (*)[18][4]>(pTempGranules[ch]), static_cast<float (*)[32]>(pTempGranules[2]));
        }

        float (*pFrac)[18][32] = reinterpret_cast<float (*)[18][32]>(pTempGranules[2]);

        for (i = 0; i < 18; i++)
        {
            if (staticDetectCPU.IsSSE())
            {
                PolySynthSSE(ch, pOutputSamplesLocal[ch], reinterpret_cast<float*>((*pFrac)[i]));
            }
            else
            {
                PolySynth(ch, pOutputSamplesLocal[ch], reinterpret_cast<float*>((*pFrac)[i]));
            }

            pOutputSamplesLocal[ch] += 32;
        }
    }

    if (unsigned int flush = static_cast<unsigned int>(GetBitPos() & 7))
    {
        GetBits(8 - flush);
    }

    cFrameSize = GetBitPos() / 8u;
    return 0;
}
