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

#include "private\mpegcommon.h"
#include "system.h"
#include "ibase.h"

CMpegBase::CMpegBase()
{
    mResetSynthRequired = 0;
    mOpened = 0;
    mHeader = 0;
    mpPolySynthHistoryF = 0;
    mpPolySynthHistoryS = 0;
    mpLoadedPolySynthHistoryF = 0;
    mpLoadedPolySynthHistoryS = 0;
}

CMpegBase::~CMpegBase()
{
    FreeSynth();
}

int CMpegBase::AllocateSynth(int numChannels)
{
    unsigned int sizeOfPolySynthHistoryF = sizeof(PolySynthHistoryF) * numChannels;
    mpPolySynthHistoryF = static_cast<PolySynthHistoryF*>(System::GetInstance()->Alloc(sizeOfPolySynthHistoryF));

    if (!mpPolySynthHistoryF)
    {
        return -1;
    }

    memset(mpPolySynthHistoryF, 0, sizeOfPolySynthHistoryF);
    return 0;
}

void CMpegBase::FreeSynth()
{
    if (mpPolySynthHistoryF)
    {
        System::GetInstance()->Free(mpPolySynthHistoryF);
    }
}

void CMpegBase::Seek(void* buf)
{
    mBufPtr = static_cast<unsigned char*>(buf);
    mBufPtrBase = mBufPtr;
    mBufPtrNext = mBufPtr;
    mShiftReg = 0;
    mShiftRegBits = 0;
}

void CMpegBase::ResetSynth()
{
    if (mpLoadedPolySynthHistoryF)
    {
        memset(mpLoadedPolySynthHistoryF, 0, sizeof(PolySynthHistoryF) * mChannels);
    }
}

const short tabsel_123[2][3][16] = { { { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 } }, { {0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }, {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, {0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 } } };

const unsigned short CMpegBase::sSampleRateTable[9] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

int CMpegBase::ProcessHeader(unsigned int hdr)
{
    int numFrames;

    if ((hdr & 0xFFE00000) != 0xFFE00000)
    {
        return -1;
    }

    mVersion = static_cast<unsigned char>((hdr >> 19) & 1);
    mLayer = static_cast<unsigned char>(4 - ((hdr >> 17) & 3));
    mErrorProt = static_cast<unsigned char>((hdr >> 16) & 1);
    mBitRateIdx = static_cast<unsigned char>((hdr >> 12) & 15);
    mPadding = static_cast<unsigned char>((hdr >> 9) & 1);
    mMode = static_cast<unsigned char>((hdr >> 6) & 3);
    mModeExt = static_cast<unsigned char>((hdr >> 4) & 3);
    mCopyright = static_cast<unsigned char>((hdr >> 3) & 1);
    mOriginal = static_cast<unsigned char>((hdr >> 2) & 1);

    if (mLayer == 4)
    {
        return -1;
    }

    if (mBitRateIdx == 15)
    {
        return -1;
    }

    if (hdr & (1 << 20))
    {
        mLSF = mVersion ? 0x0 : 0x1;
        mMPEG25 = 0;
    }
    else
    {
        mLSF = 1;
        mMPEG25 = 1;
    }

    if (mMPEG25)
    {
        mSampFreqIdx = ((hdr >> 10) & 3) + 6;
    }
    else
    {
        mSampFreqIdx = ((hdr >> 10) & 3) + 3 * mLSF;
        sfreq = ((hdr >> 10) & 3) + (mLSF != 0 ? 0 : 3);
    }

    mChannels = (mMode != 3) + 1;
    mSampFreq = sSampleRateTable[mSampFreqIdx];
    Real_mSampFreq = mSampFreq;

    if (!mBitRateIdx)
    {
        return -1;
    }

    mBitRate = tabsel_123[mLSF][mLayer - 1][mBitRateIdx];

    if (mLayer == 1)
    {
        cFrameSize = 12000 * mBitRate / Real_mSampFreq;
        cFrameSize = 4 * (cFrameSize + mPadding);
        numFrames = 384;
    }
    else
    {
        numFrames = 1152;
        cFrameSize = 144000 * mBitRate / Real_mSampFreq;

        if (mLayer == 3 && mLSF)
        {
            cFrameSize >>= 1;
            numFrames = 576;
        }

        if (mPadding)
        {
            cFrameSize++;
        }
    }

    cFrameSize -= 4;
    return numFrames;
}

int CMpegBase::DecodeHeader()
{
    mBufPtr = mBufPtrNext;
    mShiftReg = 0;
    mShiftRegBits = 0;
    unsigned int hdr;

    if (!mBufPtr)
    {
        hdr = mHeader;
    }
    else
    {
        hdr = static_cast<unsigned int>((mBufPtr[0] << 24) | (mBufPtr[1] << 16) | (mBufPtr[2] << 8) | mBufPtr[3]);
    }

    if (ProcessHeader(hdr) == -1)
    {
        return -1;
    }

    mBufPtr += 4;
    mBufPtrNext = &mBufPtr[cFrameSize];
    return 0;
}

int CMpegBase::GetHeader()
{
    unsigned int hdr;

    if (!mBufPtr)
    {
        hdr = mHeader;
    }
    else
    {
        hdr = static_cast<unsigned int>((mBufPtr[0] << 24) | (mBufPtr[1] << 16) | (mBufPtr[2] << 8) | mBufPtr[3]);
    }

    if (ProcessHeader(hdr) == -1)
    {
        return -1;
    }

    return 0;
}

void CMpegBase::Reset()
{
    mBufPtrNext = mBufPtr;
    mShiftReg = 0;
    mShiftRegBits = 0;
}

int CMpegBase::Close()
{
    if (!mOpened)
    {
        return 0;
    }

    mHeader = 0;
    mOpened = 0;
    return 0;
}
