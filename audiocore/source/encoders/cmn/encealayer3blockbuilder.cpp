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

#include "encealayer3blockbuilder.h"
#include "system.h"
#include "decoderregistry.h"
#include "decoders\decealayer32.h"
#include "encoders\lame396\lame.h"
#include "encoders\lame396\lame_global_flags.h"
#include "encoders\mp3toea.h"
#include "layer3shared.h"
#include "endian.h"
#include "decoders\ealayer31\ealayer32block.h"
#include <string.h>

void EaLayer32BlockBuilder::Init(int numChannels, int sampleRate, float quality, StartupMode startupMode, int crossFadeSamples, System* pSystem)
{
    mpSystem = pSystem;
    mNumChannels = numChannels;
    mSampleRate = sampleRate;
    mVbrQuality = quality;
    mStartupMode = startupMode;
    mCrossFadeInit = crossFadeSamples;
    mpLameEnc = 0;
    mpMp3Buffer = 0;
    mMp3BufferSize = 0;
    mpConverter = 0;
    mPcmBufferSize = mCrossFadeInit;

    if (mStartupMode == MODE_USEPCM)
    {
        mPcmBufferSize += 1105;
    }

    mpPcmBuffer = static_cast<float*>(mpSystem->Alloc(mPcmBufferSize * mNumChannels * sizeof(float)));
    mpConverter = static_cast<MP3toEA*>(mpSystem->Alloc(sizeof(MP3toEA)));
    Reset();
}

void EaLayer32BlockBuilder::Reset()
{
    mResidualSamples = 0;
    mLastSample0 = 0;
    mLastSample1 = 0;
    mMp3BufferStart = 0;
    mMp3BufferEnd = 0;

    if (mStartupMode == MODE_USEPCM)
    {
        mLatencyTotal = 1105;
    }
    else
    {
        mLatencyTotal = 0;
    }

    mLatencyCollected = 0;
    mPcmUsed = 0;
    mCrossFadeTotal = mCrossFadeInit;
    mCrossFadeCollected = 0;
    mIsCrossFadeReady = false;
    mIsFlushed = false;

    if (mpLameEnc)
    {
        lame_close(mpLameEnc);
    }

    mpLameEnc = lame_init();
    mpConverter->reset();
    mNumConvertedGranules = 0;
    mNextConvertedGranule = 0;
    mBlocksWritten = 0;
    mpLameEnc->VBR_q = static_cast<int>(9 * (1 - mVbrQuality));
    mpLameEnc->VBR = vbr_mtrh;
    mpLameEnc->in_samplerate = mSampleRate;
    mpLameEnc->out_samplerate = mSampleRate;
    mpLameEnc->quality = 2;
    mpLameEnc->bWriteVbrTag = 0;
    mpLameEnc->lowpassfreq = 0;
    mpLameEnc->num_channels = mNumChannels;

    if (mpLameEnc->num_channels == 2)
    {
        mpLameEnc->mode = JOINT_STEREO;
    }
    else
    {
        mpLameEnc->mode = MONO;
    }

    lame_init_params(mpLameEnc);
    mMp3Version = mpLameEnc->version;
}

void EaLayer32BlockBuilder::Release()
{
    if (mpLameEnc)
    {
        lame_close(mpLameEnc);
    }

    if (mpPcmBuffer)
    {
        mpSystem->Free(mpPcmBuffer);
    }

    if (mpMp3Buffer)
    {
        mpSystem->Free(mpMp3Buffer);
    }

    if (mpConverter)
    {
        mpSystem->Free(mpConverter);
    }
}

void EaLayer32BlockBuilder::ResizeMp3Buffer(int samples)
{
    unsigned int neededSpace = static_cast<unsigned int>(1.25f * (samples)+7200);
    unsigned int usableSpace = mMp3BufferSize - mMp3BufferEnd;
    unsigned int bufferedBytes = mMp3BufferEnd - mMp3BufferStart;
    unsigned int availableSpace = mMp3BufferSize - bufferedBytes;

    if (availableSpace < neededSpace)
    {
        mMp3BufferSize += 4096 + AlignUp(neededSpace, 4096);
        unsigned char* pOldBuffer = mpMp3Buffer;
        mpMp3Buffer = static_cast<unsigned char*>(mpSystem->Alloc(mMp3BufferSize));

        if (pOldBuffer != 0)
        {
            memcpy(mpMp3Buffer, &pOldBuffer[mMp3BufferStart], bufferedBytes);
            mMp3BufferEnd = bufferedBytes;
            mMp3BufferStart = 0;
            mpSystem->Free(pOldBuffer);
        }
    }
    else if (usableSpace < neededSpace)
    {
        memmove(mpMp3Buffer, &mpMp3Buffer[mMp3BufferStart], bufferedBytes);
        mMp3BufferEnd = bufferedBytes;
        mMp3BufferStart = 0;
    }

}

int EaLayer32BlockBuilder::CopyLatency(float* pSrc0, float* pSrc1, int numSamples)
{
    int latencyCopied = 0;

    if (0 < numSamples && mStartupMode == MODE_USEPCM && mLatencyCollected < mLatencyTotal)
    {
        latencyCopied = Min(numSamples, mLatencyTotal - mLatencyCollected);
        float* pDst = &mpPcmBuffer[mLatencyCollected];
        memcpy(pDst, pSrc0, latencyCopied * sizeof(float));

        if (mNumChannels == 2)
        {
            pDst = &mpPcmBuffer[mPcmBufferSize + mLatencyCollected];
            memcpy(pDst, pSrc1, latencyCopied * sizeof(float));
        }

        mLatencyCollected += latencyCopied;
    }

    return latencyCopied;
}

void EaLayer32BlockBuilder::CopyCrossfade(float* pSrc0, float* pSrc1, int numSamples)
{
    if (0 < numSamples && mCrossFadeCollected < mCrossFadeTotal)
    {
        int crossFadeNeeded = mCrossFadeTotal - mCrossFadeCollected;
        int crossFadeCopy = Min(numSamples, crossFadeNeeded);

        float* pDst = &mpPcmBuffer[mLatencyCollected + mCrossFadeCollected];
        memcpy(pDst, pSrc0, crossFadeCopy * sizeof(float));
        if (mNumChannels == 2)
        {
            pDst = &mpPcmBuffer[mPcmBufferSize + mLatencyCollected + mCrossFadeCollected];
            memcpy(pDst, pSrc1, crossFadeCopy * sizeof(float));
        }
        mCrossFadeCollected += crossFadeCopy;
    }
}

void EaLayer32BlockBuilder::FeedLame(float* pSrc0, float* pSrc1, int numSamples)
{
    if (0 < numSamples)
    {
        ResizeMp3Buffer(numSamples);
        int maxBytes = static_cast<int>(mMp3BufferSize - mMp3BufferEnd);
        int bytesWritten = lame_encode_buffer_float(mpLameEnc, pSrc0, pSrc1, numSamples, &mpMp3Buffer[mMp3BufferEnd], maxBytes);
        mMp3BufferEnd += bytesWritten;
        mLastSample0 = pSrc0[numSamples - 1];

        if (mNumChannels == 2)
        {
            mLastSample1 = pSrc1[numSamples - 1];
        }
    }
}

void EaLayer32BlockBuilder::FinishLame()
{
    if (0 < mMp3BufferStart)
    {
        unsigned int bufferedBytes = mMp3BufferEnd - mMp3BufferStart;
        memmove(mpMp3Buffer, &mpMp3Buffer[mMp3BufferStart], bufferedBytes);
        mMp3BufferEnd = bufferedBytes;
        mMp3BufferStart = 0;
    }

    int maxBytes = static_cast<int>(mMp3BufferSize - mMp3BufferEnd);
    int bytesWritten = lame_encode_finish(mpLameEnc, &mpMp3Buffer[mMp3BufferEnd], maxBytes);
    mpLameEnc = 0;
    mMp3BufferEnd += bytesWritten;
}

void EaLayer32BlockBuilder::Feed(float* pSampleData0, float* pSampleData1, int numSamples)
{
    mResidualSamples += numSamples;
    int latencyRemoved = CopyLatency(pSampleData0, pSampleData1, numSamples);
    int encodeSamples = numSamples - latencyRemoved;

    if (encodeSamples <= 0)
    {
        return;
    }

    float* pSrc0 = &pSampleData0[latencyRemoved];
    float* pSrc1 = &pSampleData1[latencyRemoved];
    CopyCrossfade(pSrc0, pSrc1, encodeSamples);
    FeedLame(pSrc0, pSrc1, encodeSamples);

    if (mIsCrossFadeReady == false && mCrossFadeCollected == mCrossFadeTotal)
    {
        DecodeCrossfade();
    }
}

void EaLayer32BlockBuilder::Flush()
{
    mLatencyTotal = mLatencyCollected;
    mCrossFadeTotal = mCrossFadeCollected;

    if (0 < mCrossFadeCollected)
    {
        const int samplesPerFeed = 576 / 4;
        float tempBuffer[samplesPerFeed * 2];
        float* pTemp0 = &tempBuffer[0];
        float* pTemp1 = &tempBuffer[samplesPerFeed];

        for (int i = 0; i < samplesPerFeed; i++)
        {
            pTemp0[i] = mLastSample0;
        }

        if (mNumChannels == 2)
        {
            for (int i = 0; i < samplesPerFeed; i++)
            {
                pTemp1[i] = mLastSample1;
            }
        }

        for (int samplesFed = 0; samplesFed < 1152; samplesFed += samplesPerFeed)
        {
            FeedLame(pTemp0, pTemp1, samplesPerFeed);
        }

        FinishLame();
    }

    mIsFlushed = true;

    if (mIsCrossFadeReady == false)
    {
        DecodeCrossfade();
    }
}

void EaLayer32BlockBuilder::DecodeCrossfade()
{
    if (mIsCrossFadeReady || mLatencyCollected < mLatencyTotal || mCrossFadeCollected < mCrossFadeTotal)
    {
        return;
    }

    if (mCrossFadeTotal == 0)
    {
        mIsCrossFadeReady = true;
        return;
    }

    int samplesNeeded = mCrossFadeTotal + 1105;
    int granulesNeeded = (samplesNeeded + 576 - 1) / 576;
    samplesNeeded = granulesNeeded * 576;
    unsigned int index = mMp3BufferStart;
    int samplesAvailable = 0;

    while (index + 4 - 1 < mMp3BufferEnd)
    {
        unsigned int packedHeader;
        ENDIAN::PutUB(packedHeader, *reinterpret_cast<int*>(&mpMp3Buffer[index]));
        MPEGAUDIOHDR parsedHeader;
        parsedHeader.version = static_cast<unsigned char>(mMp3Version);
        ParseMP3header(packedHeader, &parsedHeader);
        index += parsedHeader.framebytes;

        if (index <= mMp3BufferEnd)
        {
            samplesAvailable += parsedHeader.numframes;
        }
    }

    if (samplesNeeded <= samplesAvailable)
    {
        unsigned char* pTempBlockBuffer = static_cast<unsigned char*>(mpSystem->Alloc(static_cast<unsigned int>(granulesNeeded * 3072)));
        MP3toEA* pTempConverter = static_cast<MP3toEA*>(mpSystem->Alloc(sizeof(MP3toEA)));
        pTempConverter->reset();
        unsigned int mp3BufferPos = mMp3BufferStart;
        int blockBufferPos = 0;
        int numConvertedGranules = 0;
        int nextConvertedGranule = 0;
        int latencyRemaining = 1105;

        for (int i = 0; i < granulesNeeded; i++)
        {
            if (nextConvertedGranule == numConvertedGranules)
            {
                int bytesConsumed = pTempConverter->parse(&mpMp3Buffer[mp3BufferPos]);
                mp3BufferPos += bytesConsumed;
                numConvertedGranules = pTempConverter->getgranulecount();
                nextConvertedGranule = 0;
            }

            unsigned char* pGranuleData;
            int granuleSize;
            pTempConverter->getgranule(nextConvertedGranule, pGranuleData, granuleSize);
            nextConvertedGranule++;
            int offsetSamples = Min(latencyRemaining, EaLayer32Block::MAX_BLOCK_SIZE_SAMPLES);
            latencyRemaining -= offsetSamples;
            BlockOffsetMode offsetMode = BLOCKOFFSETMODE_IGNORE;
            blockBufferPos += EaLayer32Block::Write(&pTempBlockBuffer[blockBufferPos], pGranuleData, granuleSize, 0, 0, 0, mNumChannels, offsetMode, offsetSamples, mpSystem);
        }

        mpSystem->Free(pTempConverter);
        pTempConverter = 0;
        mpSystem->Lock();
        DecoderRegistry* pDecoderRegistry = mpSystem->GetDecoderRegistry();
        DecoderRegistry::DecoderHandle decoderHandle = pDecoderRegistry->GetDecoderHandle('L32S');
        DecoderExtended* pDecoder = pDecoderRegistry->DecoderExtendedFactory(decoderHandle, static_cast<unsigned int>(mNumChannels), 1, mpSystem);
        mpSystem->Unlock();
        float* pTempDecodeBuffer = static_cast<float*>(mpSystem->Alloc(mCrossFadeCollected * mNumChannels * sizeof(float)));
        pDecoder->Feed(pTempBlockBuffer, mCrossFadeCollected, Decoder::FEEDTYPE_NEW);
        pDecoder->Decode(pTempDecodeBuffer, mCrossFadeCollected);
        pDecoder->Release();
        pDecoder = 0;
        mpSystem->Free(pTempBlockBuffer);
        ApplyCrossFade(pTempDecodeBuffer);
        mpSystem->Free(pTempDecodeBuffer);
        mIsCrossFadeReady = true;
    }
}

void EaLayer32BlockBuilder::ApplyCrossFade(float* pDecodedData)
{
    static const float scaleFactor = 32768.0f;

    for (int i = 0; i < mCrossFadeCollected * mNumChannels; i++)
    {
        pDecodedData[i] *= scaleFactor;
    }

    float fadeRate = 1.0f / mCrossFadeCollected;
    float fadeIn = 0.0f;
    float fadeOut = 1.0f;

    if (mNumChannels == 2)
    {
        float* pSrc0 = &pDecodedData[0];
        float* pSrc1 = &pDecodedData[1];
        float* pDst0 = &mpPcmBuffer[mLatencyCollected];
        float* pDst1 = &mpPcmBuffer[mPcmBufferSize + mLatencyCollected];

        for (int i = 0; i < mCrossFadeCollected; i++)
        {
            *pDst0 = *pDst0 * fadeOut + *pSrc0 * fadeIn;
            *pDst1 = *pDst1 * fadeOut + *pSrc1 * fadeIn;
            fadeIn += fadeRate;
            fadeOut -= fadeRate;
            pDst0++;
            pDst1++;
            pSrc0 += 2;
            pSrc1 += 2;
        }
    }
    else
    {
        float* pSrc = &pDecodedData[0];
        float* pDst = &mpPcmBuffer[mLatencyCollected];

        for (int i = 0; i < mCrossFadeCollected; i++)
        {
            *pDst = *pDst * fadeOut + *pSrc * fadeIn;
            fadeIn += fadeRate;
            fadeOut -= fadeRate;
            pDst++;
            pSrc++;
        }
    }

    static const float maxValue = scaleFactor - 1;
    static const float minValue = -scaleFactor;
    float* pValue = &mpPcmBuffer[mLatencyCollected];

    for (int i = 0; i < mCrossFadeCollected * mNumChannels; i++)
    {
        if (*pValue < minValue)
        {
            *pValue = minValue;
        }
        else if (maxValue < *pValue)
        {
            *pValue = maxValue;
        }
    }
}

bool EaLayer32BlockBuilder::GetMp3FrameSize(unsigned int offset, int* pFrameSize, int* pSamples)
{
    bool isFrameReady;

    if (mpMp3Buffer == 0 || mMp3BufferEnd < offset + 4)
    {
        if (pFrameSize)
        {
            *pFrameSize = 0;
        }

        if (pSamples)
        {
            *pSamples = 0;
        }

        isFrameReady = false;
    }
    else
    {
        unsigned int packedHeader;
        ENDIAN::PutUB(packedHeader, *reinterpret_cast<int*>(&mpMp3Buffer[offset]));
        MPEGAUDIOHDR parsedHeader;
        parsedHeader.version = static_cast<unsigned char>(mMp3Version);
        ParseMP3header(packedHeader, &parsedHeader);

        if (pFrameSize)
        {
            *pFrameSize = parsedHeader.framebytes;
        }

        if (pSamples)
        {
            *pSamples = parsedHeader.numframes;
        }

        isFrameReady = (offset + parsedHeader.framebytes <= mMp3BufferEnd);
    }

    return isFrameReady;
}

bool EaLayer32BlockBuilder::IsBlockAvailable()
{
    if (mResidualSamples <= 0)
    {
        return false;
    }

    if (mIsFlushed)
    {
        return true;
    }

    if (mLatencyCollected < mLatencyTotal)
    {
        return false;
    }

    if (mIsCrossFadeReady == false)
    {
        return false;
    }

    int granulesNeeded = 1;

    if (mStartupMode == MODE_CPUSPIKE && mBlocksWritten == 0)
    {
        granulesNeeded = 2;
    }

    int granulesAvailable = mNumConvertedGranules - mNextConvertedGranule;

    if (granulesAvailable < granulesNeeded)
    {
        if (4 <= mMp3BufferEnd - mMp3BufferStart)
        {
            int bytes;
            int samples;
            unsigned int frameOffset = mMp3BufferStart;
            GetMp3FrameSize(frameOffset, &bytes, &samples);

            if (mMp3BufferStart + bytes <= mMp3BufferEnd)
            {
                granulesAvailable += samples / 576;

                if (granulesAvailable < granulesNeeded)
                {
                    frameOffset += bytes;

                    if (4 <= mMp3BufferEnd - frameOffset)
                    {
                        GetMp3FrameSize(frameOffset, &bytes, &samples);

                        if (frameOffset + bytes <= mMp3BufferEnd)
                        {
                            granulesAvailable += samples / 576;
                        }
                    }
                }
            }
        }
    }

    return granulesNeeded <= granulesAvailable;
}

void EaLayer32BlockBuilder::ConvertGranules()
{
    int bytesConsumed = mpConverter->parse(&mpMp3Buffer[mMp3BufferStart]);
    mNumConvertedGranules = mpConverter->getgranulecount();
    mNextConvertedGranule = 0;
    mMp3BufferStart += bytesConsumed;

    if (mMp3BufferStart == mMp3BufferEnd)
    {
        mMp3BufferStart = 0;
        mMp3BufferEnd = 0;
    }
}

void EaLayer32BlockBuilder::GetNextGranule(unsigned char** ppGranule, int* pSize)
{
    *ppGranule = 0;
    *pSize = 0;

    if (mNextConvertedGranule == mNumConvertedGranules && GetMp3FrameSize(mMp3BufferStart, 0, 0))
    {
        ConvertGranules();
    }

    if (mNextConvertedGranule < mNumConvertedGranules)
    {
        mpConverter->getgranule(mNextConvertedGranule, *ppGranule, *pSize);
        mNextConvertedGranule++;
    }
}

int EaLayer32BlockBuilder::GetNextGranuleSize()
{
    if (mNextConvertedGranule == mNumConvertedGranules && GetMp3FrameSize(mMp3BufferStart, 0, 0))
    {
        ConvertGranules();
    }

    int size = 0;

    if (mNextConvertedGranule < mNumConvertedGranules)
    {
        size = mpConverter->getgranulesize(mNextConvertedGranule);
    }

    return size;
}

void EaLayer32BlockBuilder::GetNextBlockSampleInfo(int* pNumSamples, int* pNumPcmSamples, int* pOffsetSamples, BlockOffsetMode* pBlockOffsetMode)
{
    int pcmAvailable = mLatencyCollected + mCrossFadeCollected - mPcmUsed;
    int pcmSamples = Min(pcmAvailable, EaLayer32Block::MAX_BLOCK_SIZE_SAMPLES);
    int numSamples = Min(mResidualSamples, EaLayer32Block::MAX_BLOCK_SIZE_SAMPLES);
    BlockOffsetMode offsetMode = BLOCKOFFSETMODE_IGNORE;
    int offsetSamples = 0;

    if (mStartupMode == MODE_CPUSPIKE)
    {
        if (mBlocksWritten == 0)
        {
            numSamples = 0;
            pcmSamples = 0;
            offsetSamples = 576;
        }
        else if (mBlocksWritten == 1)
        {
            static const int maxSamples = 47;
            numSamples = Min(numSamples, maxSamples);
            pcmSamples = Min(pcmSamples, maxSamples);
            offsetSamples = 576 - pcmSamples;
        }
    }

    *pNumSamples = numSamples;
    *pNumPcmSamples = pcmSamples;
    *pOffsetSamples = offsetSamples;
    *pBlockOffsetMode = offsetMode;
}

void EaLayer32BlockBuilder::GetNextBlockInfo(int* pNumSamples, int* pNumBytes)
{
    int pcmSamples;
    int offsetSamples;
    BlockOffsetMode offsetMode;
    GetNextBlockSampleInfo(pNumSamples, &pcmSamples, &offsetSamples, &offsetMode);
    int granuleSize = GetNextGranuleSize();
    *pNumBytes = EaLayer32Block::CalcSize(granuleSize, pcmSamples, mNumChannels, offsetSamples);
}

void EaLayer32BlockBuilder::WriteNextBlock(void* pDst)
{
    int numSamples;
    int pcmSamples;
    int offsetSamples;
    BlockOffsetMode offsetMode;
    GetNextBlockSampleInfo(&numSamples, &pcmSamples, &offsetSamples, &offsetMode);
    unsigned char* pGranule;
    int granuleSize;
    GetNextGranule(&pGranule, &granuleSize);
    float* pPcmData0 = 0;
    float* pPcmData1 = 0;

    if (0 < pcmSamples)
    {
        int index = mPcmUsed;
        pPcmData0 = &mpPcmBuffer[index];

        if (mNumChannels == 2)
        {
            pPcmData1 = &mpPcmBuffer[mPcmBufferSize + mPcmUsed];
        }
    }

    mPcmUsed += pcmSamples;
    mBlocksWritten++;
    mResidualSamples -= numSamples;

    EaLayer32Block::Write(pDst, pGranule, granuleSize, pPcmData0, pPcmData1, pcmSamples, mNumChannels, offsetMode, offsetSamples, mpSystem);
}
