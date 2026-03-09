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

#ifndef ENCEALAYER3BLOCKBUILDER_H
#define ENCEALAYER3BLOCKBUILDER_H

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
class MP3toEA;
class System;
class EaLayer32Block;
enum BlockOffsetMode;

class EaLayer32BlockBuilder
{
public:
    enum StartupMode
    {
        MODE_USEPCM = 0,
        MODE_CPUSPIKE = 1,
        MODE_MAX = 2
    };

    void Init(int numChannels, int sampleRate, float quality, StartupMode startupMode, int crossFadeSamples, System* pSystem);
    void Release();
    void Reset();
    void Feed(float* pSampleData0, float* pSampleData1, int numSamples);
    void Flush();
    bool IsBlockAvailable();
    void GetNextBlockInfo(int* pNumSamples, int* pNumBytes);
    void WriteNextBlock(void* pDst);

private:
    void ResizeMp3Buffer(int samples);
    int CopyLatency(float* pSrc0, float* pSrc1, int numSamples);
    void CopyCrossfade(float* pSrc0, float* pSrc1, int numSamples);
    void FeedLame(float* pSrc0, float* pSrc1, int numSamples);
    void FinishLame();
    void ApplyCrossFade(float* pDecodedData);
    void DecodeCrossfade();
    bool GetMp3FrameSize(unsigned int offset, int* pFrameSize, int* pSamples);
    void ConvertGranules();
    void GetNextGranule(unsigned char** ppGranule, int* pSize);
    int GetNextGranuleSize();
    void GetNextBlockSampleInfo(int* pNumSamples, int* pNumPcmSamples, int* pOffsetSamples, BlockOffsetMode* pOffsetMode);

    System* mpSystem;
    lame_global_flags* mpLameEnc;
    MP3toEA* mpConverter;
    float* mpPcmBuffer;
    unsigned char* mpMp3Buffer;
    float mLastSample0;
    float mLastSample1;
    float mVbrQuality;
    StartupMode mStartupMode;
    int mNumConvertedGranules;
    int mNextConvertedGranule;
    int mPcmBufferSize;
    int mPcmUsed;
    int mLatencyTotal;
    int mLatencyCollected;
    int mCrossFadeInit;
    int mCrossFadeTotal;
    int mCrossFadeCollected;
    int mBlocksWritten;
    unsigned int mMp3BufferSize;
    unsigned int mMp3BufferStart;
    unsigned int mMp3BufferEnd;
    int mMp3Version;
    int mNumChannels;
    int mSampleRate;
    int mResidualSamples;
    bool mIsCrossFadeReady;
    bool mIsFlushed;
};

#endif
