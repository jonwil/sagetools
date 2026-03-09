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

#ifndef DECODER_H
#define DECODER_H

#include "system.h"
#include "samplebuffer.h"

struct SndPlayerChunkHeader
{
    unsigned int bytes;
    unsigned int samples;
};

typedef unsigned int DecoderGetSizeFn(unsigned int numChannels, unsigned int* pAlignment);
typedef bool DecoderCreateInstanceEventFn(Decoder* pDecoder);
typedef void DecoderReleaseEventFn(Decoder* pDecoder);
typedef int DecoderDecodeEventFn(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples);

struct DecoderDesc
{
    DecoderGetSizeFn* pGetSize;
    DecoderCreateInstanceEventFn* pCreateInstanceEvent;
    DecoderReleaseEventFn* pReleaseEvent;
    DecoderDecodeEventFn* pDecodeEvent;
    void* listNode;
    Guid guid;
    unsigned short maxBlockSize;
};

class Decoder
{
public:
    typedef unsigned char RequestHandle;

    enum FeedType
    {
        FEEDTYPE_NEW = 0,
        FEEDTYPE_CONTINUE = 1
    };

    RequestHandle Feed(void* pSrc, int numSamples, FeedType feedType, int decoderSkip = 0, void* pSeekData = 0, int seekDataVersion = 0);
    int Decode(SampleBuffer* pDecodedSampleBuffer, int numSamples);

    int GetSamplesRemaining(RequestHandle requestHandle)
    {
        RequestDesc* pRequestDesc = GetRequestDesc(requestHandle);
        int samplesRemaining = pRequestDesc->numSamples;

        if (!samplesRemaining)
        {
            return 0;
        }

        if (requestHandle == mDecodeSlot)
        {
            samplesRemaining -= mDecodeSlotSamplesDecoded;
        }
        else
        {
            samplesRemaining -= pRequestDesc->decoderSkip;
        }

        return samplesRemaining;
    }

    void Release()
    {
        if (mpReleaseEvent)
        {
            mpReleaseEvent(this);
        }

        if (mSampleBufferStorage)
        {
            System::GetInstance()->Free(mSampleBufferStorage);
        }

        System::GetInstance()->Delete(this);
    }

    unsigned int GetInstanceSize()
    {
        return mInstanceSize;
    }

protected:
    struct RequestDesc
    {
        void* pSrc;
        void* pSeekData;
        int decoderSkip;
        int numSamples;
        unsigned char feedType;
        unsigned char seekDataVersion;
        friend class DecoderExtended;
    };

    virtual void FeedEvent(RequestHandle requestHandle) {}
    System* GetSystem() { return mpSystemUseGetSystemAccessor; }
    unsigned int GetChannels() { return mNumChannels; }

    RequestDesc* GetCurrentRequestDesc()
    {
        RequestDesc* pRequestDesc = GetRequestDesc(mPrepareSlot);

        if (!pRequestDesc->numSamples)
        {
            return 0;
        }

        AdvanceSlot(&mPrepareSlot);
        return pRequestDesc;
    }

    void ResetPrepareSlot()
    {
        mPrepareSlot = mDecodeSlot;
        AdvanceSlot(&mPrepareSlot);
    }

    const RequestDesc* GetDecodingRequestDesc()
    {
        RequestDesc* pRequestDesc = GetRequestDesc(mDecodeSlot);

        if (!pRequestDesc->numSamples)
        {
            return 0;
        }

        return pRequestDesc;
    }

    Decoder() {}
    virtual ~Decoder() {}

private:
    friend class System;
    friend class DecoderExtended;
    friend class DecoderRegistry;
    friend struct DecoderBaseModule;

    void SetSystem(System* system) { mpSystemUseGetSystemAccessor = system; }
    void AdvanceDecodeState(int numSamples);
    RequestDesc* GetRequestDescArray() { return reinterpret_cast<RequestDesc*>(reinterpret_cast<uintptr_t>(this) + mRequestDescOffset); }

    RequestDesc* GetRequestDesc(unsigned int slot)
    {
        RequestDesc* pRequestDesc = GetRequestDescArray();
        return &pRequestDesc[slot];
    }

    void SetChannels(unsigned int numChannels) { mNumChannels = static_cast<unsigned char>(numChannels); }
    SampleBuffer* GetSampleBuffer() { return reinterpret_cast<SampleBuffer*>(reinterpret_cast<uintptr_t>(this) + mSampleBufferOffset); }

    RequestDesc* AdvanceSlot(unsigned char* pSlot)
    {
        (*pSlot)++;

        if (*pSlot >= mMaxSlots)
        {
            *pSlot = 0;
        }

        RequestDesc* pRequestDescArray = GetRequestDescArray();
        return &pRequestDescArray[*pSlot];
    }

    System* mpSystemUseGetSystemAccessor;
    void* mpDecoder;
    DecoderReleaseEventFn* mpReleaseEvent;
    void* mSampleBufferStorage;
    DecoderDecodeEventFn* mpDecodeEvent;
    Guid mGuid;
    int mDecodeSlotSamplesDecoded;
    unsigned int mInstanceSize;
    unsigned int mRequestDescOffset;
    unsigned int mSampleBufferOffset;
    unsigned short mDecodedSamplesAvailable;
    unsigned char mNumChannels;
    unsigned char mFeedSlot;
    unsigned char mPrepareSlot;
    unsigned char mDecodeSlot;
    unsigned char mMaxSlots;
    unsigned char mIsBlockBased;
};

class DecoderExtended
{
public:
    int GetSamplesRemaining(Decoder::RequestHandle requestHandle) { return mpDecoder->GetSamplesRemaining(requestHandle); }
    Decoder::RequestHandle Feed(void* pSrc, int numSamples, Decoder::FeedType feedType);
    int Decode(float* pDst[], int numSamples);
    int Decode(float* pDecodedSample, int numSamples);
    int Decode(short* pDecodedSample, int numSamples);
    int Decode(short* pDecodedSample[], int numSamples);
    void Release();

private:
    friend class DecoderRegistry;
    friend class System;
    static DecoderExtended* CreateInstance(System* pSystem, unsigned int numChannels);
    ~DecoderExtended() {};
    void SetDecoder(Decoder* decoder) { mpDecoder = decoder; }
    Decoder* GetDecoder() { return mpDecoder; }
    static void TranslateF32ToS16(void* pSrc, void* pDst, int numSamples);
    Decoder* mpDecoder;
    SampleBuffer* mpSampleBuffer;
    void* mpSampleBufferStorage;
};

#endif
