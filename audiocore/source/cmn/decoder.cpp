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

#include "decoder.h"

Decoder::RequestHandle Decoder::Feed(void* pSrc, int numSamples, FeedType feedType, int decoderSkip, void* pSeekData, int seekDataVersion)
{
    RequestDesc* pRequestDesc = GetRequestDesc(mFeedSlot);

    if (pRequestDesc->numSamples)
    {
        return 0;
    }

    RequestHandle requestHandle = mFeedSlot;
    pRequestDesc->pSrc = pSrc;
    pRequestDesc->pSeekData = pSeekData;
    pRequestDesc->decoderSkip = decoderSkip;
    pRequestDesc->numSamples = numSamples;
    pRequestDesc->seekDataVersion = static_cast<unsigned char>(seekDataVersion);
    pRequestDesc->feedType = static_cast<unsigned char>(feedType);
    FeedEvent(requestHandle);

    if (mFeedSlot == mDecodeSlot)
    {
        mDecodeSlotSamplesDecoded = pRequestDesc->decoderSkip;
    }

    AdvanceSlot(&mFeedSlot);
    return requestHandle;
}

void Decoder::AdvanceDecodeState(int numSamples)
{
    RequestDesc* pRequestDesc = GetRequestDesc(mDecodeSlot);
    mDecodeSlotSamplesDecoded += numSamples;

    if (mDecodeSlotSamplesDecoded == pRequestDesc->numSamples)
    {
        pRequestDesc->numSamples = 0;
        AdvanceSlot(&mDecodeSlot);
        pRequestDesc = GetRequestDesc(mDecodeSlot);
        mDecodeSlotSamplesDecoded = pRequestDesc->decoderSkip;
    }
}

int Decoder::Decode(SampleBuffer* pDecodedSampleBuffer, int numSamples)
{
    SampleBuffer* pSampleBuffer = NULL;
    int totalSamplesDecoded = 0;

    if (mIsBlockBased)
    {
        pSampleBuffer = GetSampleBuffer();

        if (mDecodedSamplesAvailable > 0)
        {
            int maxSampleCopy = mDecodedSamplesAvailable < numSamples - totalSamplesDecoded ? mDecodedSamplesAvailable : numSamples - totalSamplesDecoded;

            for (unsigned int i = 0; i < GetChannels(); i++)
            {
                float* pSampleBufferDst = pSampleBuffer->LockChannel(i);
                float* pDestChannel = pDecodedSampleBuffer->LockChannel(i);
                memcpy(pDestChannel + totalSamplesDecoded, &pSampleBufferDst[pSampleBuffer->GetNumSamples() - mDecodedSamplesAvailable], maxSampleCopy * sizeof(float));
                pSampleBuffer->UnlockChannel(i);
                pDecodedSampleBuffer->UnlockChannel(i);
            }

            mDecodedSamplesAvailable = static_cast<unsigned short>(mDecodedSamplesAvailable - maxSampleCopy);
            totalSamplesDecoded += maxSampleCopy;
            AdvanceDecodeState(maxSampleCopy);
        }

        while (totalSamplesDecoded < numSamples)
        {
            Decoder::RequestDesc* pRequestDesc = GetRequestDesc(mDecodeSlot);

            if (!pRequestDesc->numSamples)
            {
                goto abort;
            }

            int blockSamplesDecoded;
            blockSamplesDecoded = mpDecodeEvent(this, pSampleBuffer, numSamples - totalSamplesDecoded);
            int remainingSamplesInRequest = pRequestDesc->numSamples - mDecodeSlotSamplesDecoded;
            mDecodedSamplesAvailable = static_cast<unsigned short>(blockSamplesDecoded < remainingSamplesInRequest ? blockSamplesDecoded : remainingSamplesInRequest);
            pSampleBuffer->SetNumSamples(mDecodedSamplesAvailable);
            int maxSampleCopy = mDecodedSamplesAvailable < numSamples - totalSamplesDecoded ? mDecodedSamplesAvailable : numSamples - totalSamplesDecoded;

            for (unsigned int i = 0; i < GetChannels(); i++)
            {
                float* pSampleBufferDst = pSampleBuffer->LockChannel(i);
                float* pDestChannel = pDecodedSampleBuffer->LockChannel(i);
                memcpy(pDestChannel + totalSamplesDecoded, pSampleBufferDst, maxSampleCopy * sizeof(float));
                pSampleBuffer->UnlockChannel(i);
                pDecodedSampleBuffer->UnlockChannel(i);
            }

            mDecodedSamplesAvailable = static_cast<unsigned short>(mDecodedSamplesAvailable - maxSampleCopy);
            totalSamplesDecoded += maxSampleCopy;
            AdvanceDecodeState(maxSampleCopy);
        }
    }
    else
    {
        while (totalSamplesDecoded < numSamples)
        {
            Decoder::RequestDesc* pRequestDesc = GetRequestDesc(mDecodeSlot);

            if (!pRequestDesc->numSamples)
            {
                goto abort;
            }

            int maxSamples = numSamples - totalSamplesDecoded < pRequestDesc->numSamples - mDecodeSlotSamplesDecoded ? numSamples - totalSamplesDecoded : pRequestDesc->numSamples - mDecodeSlotSamplesDecoded;
            mpDecodeEvent(this, pDecodedSampleBuffer, maxSamples);
            totalSamplesDecoded += maxSamples;
            AdvanceDecodeState(maxSamples);
        }
    }
abort:
    return totalSamplesDecoded;
}
