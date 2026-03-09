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

#include "decoders\declayer3.h"
#include "cmn\scalesamples.h"
#include "ibase.h"

DecoderDesc Layer3Dec::sDecoderDesc = { GetSize, CreateInstanceEvent, ReleaseEvent, DecodeEvent, 0, GUID, LAYER3_BLOCKSAMPLES };

Layer3Dec::Layer3Dec(int numChannels)
{
    int result;
    result = AllocateSynth(numChannels);
    result = AllocateHybrid(numChannels);
}

DecoderDesc* Layer3Dec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool Layer3Dec::CreateInstanceEvent(Decoder* pDecoder)
{
    Layer3Dec* pThis = static_cast<Layer3Dec*>(pDecoder);
    pThis->mRemainingSamples = 0;
    pThis->mpEncodedSample = 0;
    int numChannels = pThis->GetChannels();
    new (pThis) Layer3Dec(numChannels);
    return true;
}

void Layer3Dec::ReleaseEvent(Decoder* pDecoder)
{
    Layer3Dec* pThis = static_cast<Layer3Dec*>(pDecoder);
    pThis->~Layer3Dec();
}

int Layer3Dec::DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples)
{
    Layer3Dec* pThis = static_cast<Layer3Dec*>(pDecoder);

    if (pThis->mRemainingSamples <= 0)
    {
        RequestDesc* pRequestDesc = pThis->GetCurrentRequestDesc();

        if (pRequestDesc->feedType == Decoder::FEEDTYPE_NEW)
        {
            pThis->Reset();
        }

        pThis->mpEncodedSample = static_cast<unsigned char*>(pRequestDesc->pSrc);
        pThis->mRemainingSamples = pRequestDesc->numSamples;
        pThis->Open(pThis->mpEncodedSample, 0);
    }
    else
    {
        pThis->Seek(pThis->mpEncodedSample);
    }

    if (pThis->Decode(pThis->mFrameBuf) < 0)
    {
        return 0;
    }

    for (unsigned int i = 0; i < pThis->GetChannels(); i++)
    {
        float* pDst = pSampleBuffer->LockChannel(i);
        memcpy(pDst, &pThis->mFrameBuf[i * pThis->mFrameSamples], 4 * pThis->mFrameSamples);
        ScaleSamples(pDst, 1.0f / 32768.0f, pThis->mFrameSamples);
        pSampleBuffer->UnlockChannel(i);
    }

    pThis->mpEncodedSample += pThis->cFrameSize + 4;
    pThis->mRemainingSamples -= pThis->mFrameSamples;
    return pThis->mFrameSamples;
}

void Bit_Reserve::reset()
{
    mInPtr = 0;
    mOutPtr = 0;
    mCached = 0;
}

unsigned int Bit_Reserve::hsstell() const
{
    return (mOutPtr << 3) - mCached;
}

unsigned int Bit_Reserve::hgetbits(unsigned int N)
{
    unsigned int val = 0;

    while (N > 0)
    {
        if (mCached == 0)
        {
            mCacheData = static_cast<unsigned int>(mBuffer[mOutPtr & (2048 - 1)] << 24);
            mOutPtr++;
            mCached = 8;
        }

        if (N < mCached)
        {
            val = (val << N) | (mCacheData >> (32 - N));
            mCacheData <<= N;
            mCached -= N;
            break;
        }
        else
        {
            val = (val << mCached) | (mCacheData >> (32 - mCached));
            N -= mCached;
            mCached = 0;
        }
    }

    return val;
}

unsigned int Bit_Reserve::hget1bit()
{
    if (mCached == 0)
    {
        mCacheData = static_cast<unsigned int>(mBuffer[mOutPtr & (2048 - 1)] << 24);
        mOutPtr++;
        mCached = 8;
    }

    const int bit = static_cast<int>(mCacheData >> 31);
    mCacheData <<= 1;
    mCached--;
    return static_cast<unsigned int>(bit);
}

void Bit_Reserve::hputbuf(int val)
{
    mBuffer[mInPtr] = static_cast<unsigned char>(val);
    mInPtr = (mInPtr + 1) & (2048 - 1);
}

void Bit_Reserve::rewindNbits(int n)
{
    mCached += n;
    mOutPtr -= (mCached >> 3);
    mCached &= 7;

    if (mCached)
    {
        mCacheData = static_cast<unsigned int>(mBuffer[(mOutPtr - 1) & (2048 - 1)] << (32 - mCached));
    }
}

void Bit_Reserve::rewindNbytes(int N)
{
    mOutPtr -= N;

    if (mCached)
    {
        mCacheData = static_cast<unsigned int>(mBuffer[(mOutPtr - 1) & (2048 - 1)] << (32 - mCached));
    }
}

int Layer3Dec::Open(void* buf, int filesize)
{
    if (mOpened)
    {
        Close();
    }

    mOpened = true;
    mBufPtr = static_cast<unsigned char*>(buf);
    mBufPtrBase = mBufPtr;

    if (!mBufPtr)
    {
        return -1;
    }

    if (GetHeader())
    {
        return -1;
    }

    ResetSynth();
    OpenLayer();
    Reset();
    return 0;
}

bool Layer3Dec::GetSideInfo()
{
    int ch;
    int gr;

    if (!mLSF)
    {
        mpTemp->sideInfo.mainDataBegin = GetBits(9);

        if (mChannels == 1)
        {
            GetBits(5);
        }
        else
        {
            GetBits(3);
        }

        for (ch = 0; ch < mChannels; ch++)
        {
            mpTemp->sideInfo.ch[ch].scfsi[0] = static_cast<unsigned char>(GetBits(1));
            mpTemp->sideInfo.ch[ch].scfsi[1] = static_cast<unsigned char>(GetBits(1));
            mpTemp->sideInfo.ch[ch].scfsi[2] = static_cast<unsigned char>(GetBits(1));
            mpTemp->sideInfo.ch[ch].scfsi[3] = static_cast<unsigned char>(GetBits(1));
        }

        for (gr = 0; gr < 2; gr++)
        {
            for (ch = 0; ch < mChannels; ch++)
            {
                GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
                pGranuleInfo->part2And3Length = static_cast<unsigned short>(GetBits(12));
                pGranuleInfo->bigValues = static_cast<unsigned short>(GetBits(9));
                pGranuleInfo->globalGain = static_cast<unsigned char>(GetBits(8));
                pGranuleInfo->scaleFacCompress = static_cast<unsigned short>(GetBits(4));
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
                    }

                    pGranuleInfo->region1Count = 20 - pGranuleInfo->region0Count;
                }
                else
                {
                    pGranuleInfo->tableSelect[0] = static_cast<unsigned char>(GetBits(5));
                    pGranuleInfo->tableSelect[1] = static_cast<unsigned char>(GetBits(5));
                    pGranuleInfo->tableSelect[2] = static_cast<unsigned char>(GetBits(5));
                    pGranuleInfo->region0Count = static_cast<unsigned char>(GetBits(4));
                    pGranuleInfo->region1Count = static_cast<unsigned char>(GetBits(3));
                    pGranuleInfo->blockType = 0;
                    pGranuleInfo->mixedBlockFlag = 0;
                }

                pGranuleInfo->preFlag = static_cast<unsigned char>(GetBits(1));
                pGranuleInfo->scaleFacScale = GetBits(1);
                pGranuleInfo->count1TableSelect = static_cast<unsigned char>(GetBits(1));
            }
        }
    }
    else
    {
        mpTemp->sideInfo.mainDataBegin = GetBits(8);

        if (mChannels == 1)
        {
            GetBits(1);
        }
        else
        {
            GetBits(2);
        }

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
                    pGranuleInfo->region1Count = 20 - pGranuleInfo->region0Count;
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
                pGranuleInfo->mixedBlockFlag = 0;
            }

            pGranuleInfo->scaleFacScale = GetBits(1);
            pGranuleInfo->count1TableSelect = static_cast<unsigned char>(GetBits(1));
        }
    }

    return true;
}

void Layer3Dec::GetScaleFactors(unsigned int ch, unsigned int gr)
{
    int sfb;
    int window;
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    int scale_comp = pGranuleInfo->scaleFacCompress;
    unsigned int length0 = slen[0][scale_comp];
    unsigned int length1 = slen[1][scale_comp];
    Layer3ScaleFactors* pScaleFactors = &mScaleFactors[ch];

    if (pGranuleInfo->windowSwitchingFlag && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
    {
        if (pGranuleInfo->mixedBlockFlag)
        {
            for (sfb = 0; sfb < 8; sfb++)
            {
                pScaleFactors->longBlock[sfb] = static_cast<unsigned short>(br.hgetbits(slen[0][pGranuleInfo->scaleFacCompress]));
            }

            for (sfb = 3; sfb < 6; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = static_cast<unsigned short>(br.hgetbits(slen[0][pGranuleInfo->scaleFacCompress]));
                }
            }

            for (sfb = 6; sfb < 12; sfb++)
            {
                for (window = 0; window < 3; window++)
                {
                    pScaleFactors->shortBlock[window][sfb] = static_cast<unsigned short>(br.hgetbits(slen[1][pGranuleInfo->scaleFacCompress]));
                }
            }

            sfb = 12;

            for (window = 0; window < 3; window++)
            {
                pScaleFactors->shortBlock[window][sfb] = 0;
            }
        }
        else
        {
            pScaleFactors->shortBlock[0][0] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][0] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][0] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][1] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][1] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][1] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][2] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][2] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][2] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][3] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][3] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][3] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][4] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][4] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][4] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][5] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[1][5] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[2][5] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->shortBlock[0][6] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][6] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][6] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][7] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][7] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][7] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][8] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][8] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][8] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][9] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][9] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][9] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][10] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][10] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][10] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][11] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[1][11] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[2][11] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->shortBlock[0][12] = 0;
            pScaleFactors->shortBlock[1][12] = 0;
            pScaleFactors->shortBlock[2][12] = 0;
        }
    }
    else
    {
        if ((mpTemp->sideInfo.ch[ch].scfsi[0] == 0) || (gr == 0))
        {
            pScaleFactors->longBlock[0] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[1] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[2] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[3] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[4] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[5] = static_cast<unsigned short>(br.hgetbits(length0));
        }

        if ((mpTemp->sideInfo.ch[ch].scfsi[1] == 0) || (gr == 0))
        {
            pScaleFactors->longBlock[6] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[7] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[8] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[9] = static_cast<unsigned short>(br.hgetbits(length0));
            pScaleFactors->longBlock[10] = static_cast<unsigned short>(br.hgetbits(length0));
        }

        if ((mpTemp->sideInfo.ch[ch].scfsi[2] == 0) || (gr == 0))
        {
            pScaleFactors->longBlock[11] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[12] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[13] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[14] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[15] = static_cast<unsigned short>(br.hgetbits(length1));
        }

        if ((mpTemp->sideInfo.ch[ch].scfsi[3] == 0) || (gr == 0))
        {
            pScaleFactors->longBlock[16] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[17] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[18] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[19] = static_cast<unsigned short>(br.hgetbits(length1));
            pScaleFactors->longBlock[20] = static_cast<unsigned short>(br.hgetbits(length1));
        }

        pScaleFactors->longBlock[21] = 0;
        pScaleFactors->longBlock[22] = 0;
    }
}

void Layer3Dec::GetLsfScaleData(int ch, int gr, unsigned char scaleFacBuf[54])
{
    unsigned int new_slen[4] = { 0 };
    unsigned int scalefac_comp;
    unsigned int int_scalefac_comp;
    unsigned int mode_ext = mModeExt;
    int m;
    int blocktypenumber, blocknumber = 0;
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    scalefac_comp = pGranuleInfo->scaleFacCompress;

    if (pGranuleInfo->blockType == BLOCKTYPE_SHORT)
    {
        if (pGranuleInfo->mixedBlockFlag == 0)
        {
            blocktypenumber = 1;
        }
        else if (pGranuleInfo->mixedBlockFlag == 1)
        {
            blocktypenumber = 2;
        }
        else
        {
            blocktypenumber = 0;
        }
    }
    else
    {
        blocktypenumber = 0;
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
            blocknumber = 0;
        }
        else if (scalefac_comp < 500)
        {
            new_slen[0] = ((scalefac_comp - 400) >> 2) / 5;
            new_slen[1] = ((scalefac_comp - 400) >> 2) % 5;
            new_slen[2] = (scalefac_comp - 400) & 3;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blocknumber = 1;
        }
        else if (scalefac_comp < 512)
        {
            new_slen[0] = (scalefac_comp - 500) / 3;
            new_slen[1] = (scalefac_comp - 500) % 3;
            new_slen[2] = 0;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 1;
            blocknumber = 2;
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
            blocknumber = 3;
        }
        else if (int_scalefac_comp < 244)
        {
            new_slen[0] = ((int_scalefac_comp - 180) & 0x3F) >> 4;
            new_slen[1] = ((int_scalefac_comp - 180) & 0xF) >> 2;
            new_slen[2] = (int_scalefac_comp - 180) & 3;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blocknumber = 4;
        }
        else if (int_scalefac_comp < 255)
        {
            new_slen[0] = (int_scalefac_comp - 244) / 3;
            new_slen[1] = (int_scalefac_comp - 244) % 3;
            new_slen[2] = 0;
            new_slen[3] = 0;
            pGranuleInfo->preFlag = 0;
            blocknumber = 5;
        }
    }

    for (unsigned int x = 0; x < 45; x++)
    {
        scaleFacBuf[x] = 0;
    }

    m = 0;

    for (unsigned int i = 0; i < 4; i++)
    {
        for (unsigned int j = 0; j < sNumSfbBlock[blocknumber][blocktypenumber][i]; j++)
        {
            scaleFacBuf[m] = static_cast<unsigned char>((new_slen[i] == 0) ? 0 : br.hgetbits(new_slen[i]));
            m++;
        }
    }
}

void Layer3Dec::GetLsfScaleFactors(int ch, int gr)
{
    unsigned int m = 0;
    unsigned int sfb, window;
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
            {
                pScaleFactors->shortBlock[window][12] = 0;
            }
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

            for (window = 0; window < 3; window++)
            {
                pScaleFactors->shortBlock[window][12] = 0;
            }
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

void Layer3Dec::DecodeHuffman(int ch, int gr, float output[32 * 18], int part2_start)
{
    GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];
    int x;
    int y;
    int part2_3_end = part2_start + pGranuleInfo->part2And3Length;
    int num_bits;
    int region1Start;
    int region2Start;
    short index;
    HuffTable* pTable;
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
    float globalGain = gTwoToNegativeQuarterPower[(pGranuleInfo->globalGain - 255) * -1];
    float negativeGlobalGain = -globalGain;

    for (int i = 0; i < bigValues; i += 2)
    {
        int tableSelect;

        if (i < region1Start)
        {
            tableSelect = pGranuleInfo->tableSelect[0];
        }
        else if (i < region2Start)
        {
            tableSelect = pGranuleInfo->tableSelect[1];
        }
        else
        {
            tableSelect = pGranuleInfo->tableSelect[2];
        }

        pTable = &mHuffTables[tableSelect];
        int linearBits = sHuffTableLinearBits[tableSelect];

        if (pTable->pEntries)
        {
            const short* pEntry = pTable->pEntries;

            while ((y = *pEntry++) < 0)
            {
                if (br.hget1bit())
                {
                    pEntry -= y;
                }
            }

            x = y >> 4;
            y &= 0xf;

            if (x == 15 && linearBits)
            {
                x += static_cast<int>(br.hgetbits(static_cast<unsigned int>(linearBits)));
            }

            if (x)
            {
                if (br.hget1bit())
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
            }
            else
            {
                output[index] = 0.0f;
            }

            if (y == 15 && linearBits)
            {
                y += static_cast<int>(br.hgetbits(static_cast<unsigned int>(linearBits)));
            }

            if (y)
            {
                if (br.hget1bit())
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
            }
            else
            {
                output[index + 1] = 0.0f;
            }
        }
        else
        {
            output[index] = 0.0f;
            output[index + 1] = 0.0f;
        }

        index += 2;
    }

    SToPowerOf4over3(SToPowerOf4over3Count, SToPowerOf4over3Input, SToPowerOf4over3Results);

    for (int k = 0; k < SToPowerOf4over3Count; k++)
    {
        output[SToPowerOf4over3Index[k]] *= SToPowerOf4over3Results[k];
    }

    const HuffCountTable* pCountTable = &sHuffCountTables[pGranuleInfo->count1TableSelect];
    num_bits = br.hsstell();

    while ((num_bits < part2_3_end) && (index < 576))
    {
        const HuffEntry* pEntry = &pCountTable->pEntries[br.hgetbits(pCountTable->maxCodeBits)];
        br.rewindNbits(static_cast<unsigned int>(pCountTable->maxCodeBits - pEntry->length));

        if (pEntry->value & 8)
        {
            if (br.hget1bit())
            {
                output[index] = negativeGlobalGain;
            }
            else
            {
                output[index] = globalGain;
            }
        }
        else
        {
            output[index] = 0.0f;
        }

        if (pEntry->value & 4)
        {
            if (br.hget1bit())
            {
                output[index + 1] = negativeGlobalGain;
            }
            else
            {
                output[index + 1] = globalGain;
            }
        }
        else
        {
            output[index + 1] = 0.0f;
        }

        if (pEntry->value & 2)
        {
            if (br.hget1bit())
            {
                output[index + 2] = negativeGlobalGain;
            }
            else
            {
                output[index + 2] = globalGain;
            }
        }
        else
        {
            output[index + 2] = 0.0f;
        }

        if (pEntry->value & 1)
        {
            if (br.hget1bit())
            {
                output[index + 3] = negativeGlobalGain;
            }
            else
            {
                output[index + 3] = globalGain;
            }
        }
        else
        {
            output[index + 3] = 0.0f;
        }

        index += 4;
        num_bits = br.hsstell();
    }

    if (num_bits > part2_3_end)
    {
        br.rewindNbits(num_bits - part2_3_end);
        index -= 4;
    }

    num_bits = br.hsstell();

    if (num_bits < part2_3_end)
    {
        br.hgetbits(part2_3_end - num_bits);
    }

    if (index < 576)
    {
        memset(&output[index], 0, (576 - index) * sizeof(output[0]));
    }
}

int Layer3Dec::Decode(float* pOutputSamples)
{
    struct TempMemory
    {
        int temp2304Bytes[3][CMpegLayer3Base::SAMPLES_PER_GRANULE];
    };

    TempMemory* pTempMemory;
    __declspec(align(16)) TempMemory tempMemory;
    Layer3Temp layer3Temp;
    int nSlots;
    unsigned int flush_main;
    unsigned int gr;
    unsigned int ch;
    int main_data_end;
    int bytes_to_discard;
    float* pOutSamples[2];
    int i;

    pTempMemory = &tempMemory;
    mpTemp = &layer3Temp;
    pOutSamples[0] = pOutputSamples;
    pOutSamples[1] = pOutputSamples + LAYER3_BLOCKSAMPLES;

    if (DecodeHeader())
    {
        return -1;
    }

    if (mErrorProt == 0)
    {
        GetBits(16);
    }

    GetSideInfo();

    if (!mLSF)
    {
        nSlots = (cFrameSize - ((mMode == 3u) ? 17u : 32u) - (mErrorProt ? 0u : 2u));
    }
    else
    {
        nSlots = (cFrameSize - ((mMode == 3u) ? 9u : 17u) - (mErrorProt ? 0u : 2u));
    }

    for (i = 0; i < (int)nSlots; i++)
    {
        br.hputbuf(static_cast<int>(GetBits(8)));
    }

    main_data_end = static_cast<int>(br.hsstell() >> 3);
    flush_main = (br.hsstell() & 7);

    if (flush_main)
    {
        br.hgetbits(8 - flush_main);
        main_data_end++;
    }

    bytes_to_discard = static_cast<int>(mFrameStart - main_data_end - mpTemp->sideInfo.mainDataBegin);
    mFrameStart += nSlots;

    if (bytes_to_discard < 0)
    {
        return -1;
    }

    if (main_data_end > 2048)
    {
        mFrameStart -= 2048;
        br.rewindNbytes(2048);
    }

    for (; bytes_to_discard > 0; bytes_to_discard--)
    {
        br.hgetbits(8);
    }

    for (gr = 0; gr < max_gr; gr++)
    {
        void* p2304Bytes[3];
        p2304Bytes[0] = pTempMemory->temp2304Bytes[0];
        p2304Bytes[1] = pTempMemory->temp2304Bytes[1];
        p2304Bytes[2] = pTempMemory->temp2304Bytes[2];
        mpLoadedTwoToNegativeQuarterPower = mpTwoToNegativeQuarterPower;

        for (ch = 0; ch < mChannels; ch++)
        {
            const unsigned int part2_start = br.hsstell();

            if (mVersion == 1)
            {
                GetScaleFactors(ch, gr);
            }
            else
            {
                GetLsfScaleFactors(static_cast<int>(ch), static_cast<int>(gr));
            }

            sHuffCountTables[0].pEntries = gHuffTableCount0;
            sHuffCountTables[0].maxCodeBits = 6;
            sHuffCountTables[0].maxCodeShifter = 26;
            sHuffCountTables[1].pEntries = gHuffTableCount1;
            sHuffCountTables[1].maxCodeBits = 4;
            sHuffCountTables[1].maxCodeShifter = 28;
            DecodeHuffman(ch, gr, (float*)p2304Bytes[ch], part2_start);
            Dequantize(ch, gr, (float*)p2304Bytes[ch]);
        }

        if (mChannels == 2)
        {
            unsigned int is_pos[576];
            float is_rat_io[576];
            K k;
            mpIs_pos = is_pos;
            mpIs_rat_io = is_rat_io;
            mpK = &k;
            Stereo(gr, reinterpret_cast<float (*)[32][18]>(p2304Bytes[0]));
        }

        for (ch = 0; ch < mChannels; ch++)
        {
            const GranuleInfo* pGranuleInfo = &mGranuleInfo[ch][gr];

            if (pGranuleInfo->windowSwitchingFlag && (pGranuleInfo->blockType == BLOCKTYPE_SHORT))
            {
                Reorder(ch, gr, static_cast<float*>(p2304Bytes[ch]), static_cast<float*>(p2304Bytes[2]));
            }
            else
            {
                void* pTemp = p2304Bytes[ch];
                p2304Bytes[ch] = p2304Bytes[2];
                p2304Bytes[2] = pTemp;
            }

            AntiAlias(ch, gr, static_cast<float*>(p2304Bytes[2]));
            ReorderForVectoring(static_cast<float*>(p2304Bytes[2]), reinterpret_cast<float (*)[18][4]>(p2304Bytes[ch]));
            mpLoadedPrevBlockX4 = mpPrevBlockX4;
            Hybrid(ch, gr, reinterpret_cast<float (*)[18][4]>(p2304Bytes[ch]));
            mpLoadedPolySynthHistoryF = mpPolySynthHistoryF;
            FrequencyInversionX4(reinterpret_cast<float (*)[18][4]>(p2304Bytes[ch]));
            ReorderForFPolySynth(reinterpret_cast<float (*)[18][4]>(p2304Bytes[ch]), static_cast<float (*)[32]>(p2304Bytes[2]));
            float (*pFrac)[18][32] = reinterpret_cast<float (*)[18][32]>(p2304Bytes[2]);

            for (i = 0; i < 18; i++)
            {
                PolySynth(ch, pOutSamples[ch], reinterpret_cast<float*>((*pFrac)[i]));
                pOutSamples[ch] += 32;
            }
        }
    }

    return 0;
}

int Layer3Dec::OpenLayer()
{
    mFrameSamples = 1152;

    if (mLSF)
    {
        mFrameSamples >>= 1;
    }

    mBandOffset[0] = 1;
    mBandOffset[1] = 1;
    max_gr = (mLSF == 0) + 1;
    mFrameStart = 0;
    br.reset();
    return 0;
}
