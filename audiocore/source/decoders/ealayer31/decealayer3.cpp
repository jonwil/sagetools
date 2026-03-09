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

#include "decoders\decealayer31.h"
#include "decoders\decealayer32.h"
#include "decoders\ealayer31\ealayer32block.h"
#include "cmn\scalesamples.h"
#include "endian.h"
#include "ibase.h"
#include <string.h>

DecoderDesc EaLayer31Dec::sDecoderDesc = { GetSize, CreateInstanceEvent, ReleaseEvent, DecodeEvent, 0, GUID, EALAYER3_BLOCKSAMPLES };

DecoderDesc* EaLayer31Dec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool EaLayer31Dec::CreateInstanceEvent(Decoder* pDecoder)
{
    return CreateInstance(pDecoder, USE_EALAYER31);
}

DecoderDesc EaLayer32PcmDec::sDecoderDesc = { GetSize, CreateInstanceEvent, ReleaseEvent, DecodeEvent, 0, GUID, EALAYER3_BLOCKSAMPLES };

DecoderDesc* EaLayer32PcmDec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool EaLayer32PcmDec::CreateInstanceEvent(Decoder* pDecoder)
{
    return CreateInstance(pDecoder, USE_EALAYER32PCM);
}

DecoderDesc EaLayer32SpikeDec::sDecoderDesc = { GetSize, CreateInstanceEvent, ReleaseEvent, DecodeEvent, 0, GUID, EALAYER3_BLOCKSAMPLES };

DecoderDesc* EaLayer32SpikeDec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool EaLayer32SpikeDec::CreateInstanceEvent(Decoder* pDecoder)
{
    return CreateInstance(pDecoder, USE_EALAYER32SPIKE);
}

unsigned int EaLayer3DecBase::GetSize(unsigned int numChannels, unsigned int* pAlignment)
{
    *pAlignment = 16;
    unsigned int memRequired = sizeof(EaLayer3DecBase);
    int numEaLayer3CoreInstances = (numChannels + 1) >> 1;
    LinearAllocAddSize(memRequired, numEaLayer3CoreInstances * 4, 1);
    return memRequired;
}

bool EaLayer3DecBase::CreateInstance(Decoder* pDecoder, char version)
{
    EaLayer3DecBase* pThis = static_cast<EaLayer3DecBase*>(pDecoder);
    pThis->mRemainingSamples = 0;
    pThis->mpEncodedSample = 0;
    pThis->mVersion = static_cast<unsigned char>(version);
    pThis->mTotalChannels = pThis->GetChannels();
    pThis->mNumEaLayer3CoreInstances = (pThis->mTotalChannels + 1) / 2;
    uintptr_t pMemBlock;
    pMemBlock = reinterpret_cast<uintptr_t>(pThis) + sizeof(*pThis);
    LinearAlloc(pThis->mppEaLayer3Core, pMemBlock, pThis->mNumEaLayer3CoreInstances * sizeof(EALayer3Core*), 1);
    unsigned int sizeOfEALayer3Core = sizeof(EALayer3Core);
    unsigned int memRequired = 0;

    for (int i = 0; i < pThis->mNumEaLayer3CoreInstances; i++)
    {
        LinearAllocAddSize(memRequired, sizeOfEALayer3Core, 16);
    }

    EALayer3Core* pEALayer3Cores = static_cast<EALayer3Core*>(pThis->GetSystem()->Alloc(memRequired));

    for (int i = 0; i < pThis->mNumEaLayer3CoreInstances; i++)
    {
        unsigned int numChannelsThisDecoder;

        if (i == pThis->mTotalChannels / 2)
        {
            numChannelsThisDecoder = 1;
        }
        else
        {
            numChannelsThisDecoder = 2;
        }

        LinearAlloc(pThis->mppEaLayer3Core[i], pEALayer3Cores, sizeOfEALayer3Core, 16);
        memset(pThis->mppEaLayer3Core[i], 0, sizeOfEALayer3Core);
        pThis->mppEaLayer3Core[i] = new(pThis->mppEaLayer3Core[i]) EALayer3Core(numChannelsThisDecoder);
    }

    pThis->mNewFeed = false;
    pThis->mLatency = MPEG_AUDIO_LAYER_LATENCY[3];
    pThis->mSkipSamples = 0;
    return true;
}

void EaLayer3DecBase::ReleaseEvent(Decoder* pDecoder)
{
    EaLayer3DecBase* pThis = static_cast<EaLayer3DecBase*>(pDecoder);

    if (*pThis->mppEaLayer3Core)
    {
        for (int i = 0; i < pThis->mNumEaLayer3CoreInstances; i++)
        {
            pThis->mppEaLayer3Core[i]->~EALayer3Core();
        }

        pThis->GetSystem()->Free(*pThis->mppEaLayer3Core);
    }
}

static const int SAMPLES_PER_GRANULE = CMpegLayer3Base::SAMPLES_PER_GRANULE;

void EaLayer3DecBase::Reset()
{
    mLatency = MPEG_AUDIO_LAYER_LATENCY[3];
    mSkipSamples = 0;
}

void EaLayer3DecBase::SkipBlocks()
{
    if (0 < mSkipSamples)
    {
        if (mNewFeed && mSkipSamples < 2 * SAMPLES_PER_GRANULE - MPEG_AUDIO_LAYER_LATENCY[3] && mVersion != USE_EALAYER32PCM)
        {
            mLatency -= SAMPLES_PER_GRANULE;
        }
        else
        {
            mLatency = 0;
        }

        int blocksToSkip;
        int samplesSkipped = 0;

        if (mNewFeed && mVersion != USE_EALAYER32PCM)
        {
            blocksToSkip = (mSkipSamples + MPEG_AUDIO_LAYER_LATENCY[3]) / SAMPLES_PER_GRANULE;

            if (1 < blocksToSkip)
            {
                samplesSkipped += 2 * SAMPLES_PER_GRANULE - MPEG_AUDIO_LAYER_LATENCY[3];
                samplesSkipped += SAMPLES_PER_GRANULE * (blocksToSkip - 2);
            }
        }
        else
        {
            blocksToSkip = mSkipSamples / SAMPLES_PER_GRANULE;
            samplesSkipped = blocksToSkip * SAMPLES_PER_GRANULE;
        }

        mSkipSamples -= samplesSkipped;
        static const int BLOCKS_PER_GROUP = 10;
        int coarseBlocks = (blocksToSkip - 1) / BLOCKS_PER_GROUP;

        if (mVersion == USE_EALAYER31)
        {
            coarseBlocks = blocksToSkip;
        }

        int blockOffset = 0;
        unsigned short* pBlockGroupLength = static_cast<unsigned short*>(GetDecodingRequestDesc()->pSeekData);

        for (int i = 0; i < coarseBlocks; i++)
        {
            short blockGroupLength = 0;
            ENDIAN::PutB(blockGroupLength, *pBlockGroupLength++);
            blockOffset += blockGroupLength;
        }

        int fineBlocks = blocksToSkip - coarseBlocks * BLOCKS_PER_GROUP;

        if (0 < fineBlocks)
        {
            unsigned char* pCurrentBlock = static_cast<unsigned char*>(&mpEncodedSample[blockOffset]);

            for (int blockSet = 0; blockSet < fineBlocks; blockSet++)
            {
                for (int pairIndex = 0; pairIndex < mNumEaLayer3CoreInstances; pairIndex++)
                {
                    int blockSize = EaLayer32Block::ReadBlockSize(pCurrentBlock);
                    pCurrentBlock += blockSize;
                    blockOffset += blockSize;
                }
            }
        }

        mpEncodedSample += blockOffset;
    }
}

void EaLayer3DecBase::DecodePcm(float** ppDst, void* pSrc, int numChannels, int numSamples)
{
    if (numChannels == 2)
    {
        short* pPcmData0 = static_cast<short*>(pSrc);
        short* pPcmData1 = pPcmData0 + 1;
        float* pDst0 = ppDst[0];
        float* pDst1 = ppDst[1];

        for (int i = 0; i < numSamples; i++)
        {
            short sample0;
            short sample1;
            ENDIAN::PutUB(sample0, *pPcmData0);
            ENDIAN::PutUB(sample1, *pPcmData1);
            *pDst0 = static_cast<float>(sample0);
            *pDst1 = static_cast<float>(sample1);
            pDst0++;
            pDst1++;
            pPcmData0 += 2;
            pPcmData1 += 2;
        }
    }
    else
    {
        short* pPcmData = static_cast<short*>(pSrc);
        float* pDst = ppDst[0];

        for (int i = 0; i < numSamples; i++)
        {
            short sample;
            ENDIAN::PutUB(sample, *pPcmData);
            *pDst = static_cast<float>(sample);
            pDst++;
            pPcmData++;
        }
    }
}

int EaLayer3DecBase::DecodeSpecialBlock(unsigned char* pSrc, float** ppOutputSamples, EALayer3Core* pEALayer3CoreDecoder)
{
    int rawSamples;
    int rawOffset;
    int* pSrcInt = reinterpret_cast<int*>(pSrc);
    ENDIAN::PutUB(rawSamples, *pSrcInt++);
    ENDIAN::PutUB(rawOffset, *pSrcInt++);
    short rawValue;
    short* pSrcShort = reinterpret_cast<short*>(pSrcInt);

    for (int ch = 0; ch < pEALayer3CoreDecoder->mChannels; ch++)
    {
        for (int i = 0; i < rawSamples; i++)
        {
            ENDIAN::PutUB(rawValue, *pSrcShort++);
            ppOutputSamples[ch][i + rawOffset] = static_cast<float>(rawValue);
        }
    }

    int bytesConsumed = 2 * sizeof(unsigned int) + rawSamples * pEALayer3CoreDecoder->mChannels * sizeof(short);
    return bytesConsumed;
}

int EaLayer3DecBase::DecodeGranule(unsigned char* pSrc, float** ppOutputSamples, EALayer3Core* pEALayer3CoreDecoder, int* pFramesDecoded, int* pLatencyConsumed, int* pSkipConsumed, int numChannels)
{
    EaLayer32Block block;
    unsigned char* pGranule;
    int bytesConsumed = 0;
    int specialBlock = 0;

    if (mVersion == USE_EALAYER31)
    {
        if (*pSrc == 0xEE)
        {
            specialBlock = 1;
        }

        bytesConsumed = 1;
        pGranule = &pSrc[1];
    }
    else
    {
        block.Init(GetSystem());
        bytesConsumed = block.Read(pSrc);
        int offset = block.GetGranuleOffset();

        if (offset != 0)
        {
            pGranule = pSrc + offset;
        }
        else
        {
            pGranule = 0;
        }
    }

    if (pGranule)
    {
        if (mNewFeed)
        {
            pEALayer3CoreDecoder->Open(pGranule, 0);
        }
        else
        {
            pEALayer3CoreDecoder->Seek(pGranule);
        }

        int retcode = pEALayer3CoreDecoder->Decode(ppOutputSamples);

        if (retcode < 0)
        {
            for (int ch = 0; ch < numChannels; ch++)
            {
                memset(ppOutputSamples[ch], 0, 4 * pEALayer3CoreDecoder->mFrameSamples);
            }
        }
    }

    int decodedStart = 0;
    int muteSamples = 0;

    if (mVersion == USE_EALAYER31)
    {
        *pFramesDecoded = EALAYER3_BLOCKSAMPLES;
        *pLatencyConsumed = 0;

        if (0 < mLatency)
        {
            if (mLatency < *pFramesDecoded)
            {
                *pLatencyConsumed = mLatency;
                *pFramesDecoded -= mLatency;
                decodedStart = mLatency;
            }
            else
            {
                *pLatencyConsumed = *pFramesDecoded;
                *pFramesDecoded = 0;
            }
        }
    }
    else
    {
        *pFramesDecoded = block.GetUsableSamples();

        switch (block.GetBlockOffsetMode())
        {
        case BLOCKOFFSETMODE_IGNORE:
            decodedStart = block.GetOffsetSamples();
            break;
        case BLOCKOFFSETMODE_MUTE:
            muteSamples = block.GetOffsetSamples();
            break;
        }
    }

    if (0 < mSkipSamples)
    {
        if (mSkipSamples < *pFramesDecoded)
        {
            *pSkipConsumed = mSkipSamples;
            *pFramesDecoded -= mSkipSamples;
            decodedStart += mSkipSamples;
        }
        else
        {
            *pSkipConsumed = *pFramesDecoded;
            *pFramesDecoded = 0;
        }
    }

    if (mVersion == USE_EALAYER31)
    {
        bytesConsumed += pEALayer3CoreDecoder->cFrameSize;

        if (specialBlock)
        {
            float* pSpecialSamples[2];

            for (int i = 0; i < numChannels; i++)
            {
                pSpecialSamples[i] = ppOutputSamples[i] + *pLatencyConsumed;
            }

            bytesConsumed += DecodeSpecialBlock(&pSrc[bytesConsumed], &pSpecialSamples[0], pEALayer3CoreDecoder);
        }
    }
    else if (mVersion != USE_EALAYER31 && 0 < block.GetPcmSamples())
    {
        void* pPcmDataLocal = pSrc + block.GetPcmDataOffset();
        float* pPcmSamples[2];

        for (int i = 0; i < numChannels; i++)
        {
            pPcmSamples[i] = ppOutputSamples[i] + block.GetOffsetSamples();
        }

        DecodePcm(pPcmSamples, pPcmDataLocal, static_cast<unsigned int>(numChannels), static_cast<unsigned int>(block.GetPcmSamples()));

    }

    if (0 < *pFramesDecoded && 0 < decodedStart)
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            memmove(ppOutputSamples[ch], &ppOutputSamples[ch][decodedStart], *pFramesDecoded * sizeof(float));
        }
    }

    int muteRemaining = muteSamples - decodedStart;

    if (0 < muteRemaining)
    {
        for (int ch = 0; ch < numChannels; ch++)
        {
            memset(ppOutputSamples[ch], 0, muteRemaining * sizeof(float));
        }
    }

    return bytesConsumed;
}

int EaLayer3DecBase::DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples)
{
    EaLayer3DecBase* pThis = static_cast<EaLayer3DecBase*>(pDecoder);

    if (pThis->mRemainingSamples <= 0)
    {
        RequestDesc* pRequestDesc = pThis->GetCurrentRequestDesc();

        if (pRequestDesc->feedType == Decoder::FEEDTYPE_NEW)
        {
            pThis->mNewFeed = true;
            pThis->Reset();
        }

        pThis->mpEncodedSample = static_cast<unsigned char*>(pRequestDesc->pSrc);
        pThis->mSkipSamples = static_cast<int>(pRequestDesc->decoderSkip);
        pThis->mRemainingSamples = pRequestDesc->numSamples - pThis->mSkipSamples;

        if (0 < pThis->mSkipSamples)
        {
            pThis->SkipBlocks();
        }
    }

    int latencyConsumed = 0;
    int skipConsumed = 0;
    int samplesDecoded = 0;

    while (samplesDecoded <= 0)
    {
        unsigned char* pStartEncodedSample = pThis->mpEncodedSample;

        for (int i = 0; i < pThis->mNumEaLayer3CoreInstances; i++)
        {
            unsigned char* pSrc;
            pThis->mpLoadedEALayer3Core = pThis->mppEaLayer3Core[i];
            pSrc = pThis->mpEncodedSample;
            int numChannelsThisDecoder;

            if (i == pThis->mTotalChannels / 2)
            {
                numChannelsThisDecoder = 1;
            }
            else
            {
                numChannelsThisDecoder = 2;
            }

            float* ppOutputSamples[2];

            for (int j = (2 * i); j < (2 * i) + numChannelsThisDecoder; j++)
            {
                ppOutputSamples[j - (2 * i)] = pSampleBuffer->LockChannel(static_cast<unsigned int>(j));
            }

            int bytesConsumed = pThis->DecodeGranule(pSrc, ppOutputSamples, pThis->mpLoadedEALayer3Core, &samplesDecoded, &latencyConsumed, &skipConsumed, numChannelsThisDecoder);

            for (int j = (2 * i); j < (2 * i) + numChannelsThisDecoder; j++)
            {
                pSampleBuffer->UnlockChannel(j);
            }

            if (bytesConsumed < 0)
            {
                for (unsigned int i = 0; i < pThis->GetChannels(); i++)
                {
                    float* pDst = pSampleBuffer->LockChannel(i);
                    memset(pDst, 0, EALAYER3_BLOCKSAMPLES * sizeof(float));
                    pSampleBuffer->UnlockChannel(i);
                }

                pThis->mpEncodedSample = pStartEncodedSample;
                return -1;
            }
            else
            {
                pThis->mpEncodedSample += bytesConsumed;
            }
        }

        if (pThis->mNewFeed)
        {
            pThis->mNewFeed = false;
        }

        if (pThis->mLatency > 0)
        {
            pThis->mLatency -= latencyConsumed;
        }

        if (pThis->mSkipSamples > 0)
        {
            pThis->mSkipSamples -= skipConsumed;
        }

        for (unsigned int i = 0; i < pThis->GetChannels(); i++)
        {
            float* pDst = pSampleBuffer->LockChannel(i);
            ScaleSamples(pDst, 1.0f / 32768.0f, samplesDecoded);
            pSampleBuffer->UnlockChannel(i);
        }
    }

    if (pThis->mRemainingSamples < samplesDecoded)
    {
        samplesDecoded = pThis->mRemainingSamples;
    }

    if (pThis->mRemainingSamples >= 0)
    {
        pThis->mRemainingSamples -= samplesDecoded;
    }

    return samplesDecoded;
}
