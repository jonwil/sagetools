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

#include "decoderregistry.h"
#include "ibase.h"
#include "decoders\decealayer31.h"
#include "decoders\decealayer32.h"
#include "decoders\declayer3.h"
#include "decoders\decpcm16big.h"
#include "decoders\decpcm16little.h"
#include "decoders\decxas1.h"

void DecoderRegistry::RegisterAllDecoders()
{
    RegisterDecoder(EaLayer32PcmDec::GetDecoderDesc());
    RegisterDecoder(EaLayer32SpikeDec::GetDecoderDesc());
    RegisterDecoder(EaLayer31Dec::GetDecoderDesc());
    RegisterDecoder(Layer3Dec::GetDecoderDesc());
    RegisterDecoder(Xas1Dec::GetDecoderDesc());
    RegisterDecoder(Pcm16BigDec::GetDecoderDesc());
    RegisterDecoder(Pcm16LittleDec::GetDecoderDesc());
}

DecoderRegistry* DecoderRegistry::CreateInstance(System* pSystem)
{
    DecoderRegistry* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        return pThis;
    }

    pThis->mpSystem = pSystem;
    return pThis;
}

void DecoderRegistry::Release()
{
    mpSystem->Delete(this);
}

DecoderRegistry::DecoderHandle DecoderRegistry::GetDecoderHandle(Guid guid)
{
    ListNode* pNext = mDecoderDescList.GetHead();

    while (pNext)
    {
        DecoderDesc* pDecoderDesc = GetDecoderDescFromNode(pNext);
        pNext = pNext->GetNext();

        if (pDecoderDesc->guid == guid)
        {
            return pDecoderDesc;
        }
    }

    return 0;
}

DecoderRegistry::DecoderHandle DecoderRegistry::RegisterDecoder(DecoderDesc* pDecoderDesc)
{
    ListNode* pNext = mDecoderDescList.GetHead();

    while (pNext)
    {
        DecoderDesc* pNode = GetDecoderDescFromNode(pNext);
        pNext = pNext->GetNext();

        if (pNode->guid == pDecoderDesc->guid)
        {
            return pNode;
        }
    }

    mDecoderDescList.Push(static_cast<ListNode*>(static_cast<void*>(&pDecoderDesc->listNode)));
    return pDecoderDesc;
}

Decoder* DecoderRegistry::DecoderFactory(DecoderHandle decoderHandle, unsigned int numChannels, unsigned int maxRequests, System* pSystem)
{
    DecoderDesc* pDecoderDesc = static_cast<DecoderDesc*>(decoderHandle);
    unsigned int decoderAlignment;
    unsigned int decoderMemRequired = pDecoderDesc->pGetSize(numChannels, &decoderAlignment);
    unsigned int memRequired = decoderMemRequired;
    const unsigned int requestDescMemRequired = maxRequests * sizeof(Decoder::RequestDesc);
    LinearAllocAddSize(memRequired, requestDescMemRequired);
    unsigned char isBlockBased = static_cast<unsigned char>(pDecoderDesc->maxBlockSize ? 1u : 0u);
    unsigned int minAlignment = decoderAlignment;
    Decoder::RequestDesc* pRequestDescArray;

    if (isBlockBased)
    {
        unsigned int sampleBufferAlignment;
        unsigned int sampleBufferInstanceSize;
        sampleBufferInstanceSize = SampleBuffer::GetSize(numChannels, DEFAULT_SAMPLEBUFFER_MAXLOCKS, pDecoderDesc->maxBlockSize, &sampleBufferAlignment, pSystem);
        LinearAllocAddSize(memRequired, sampleBufferInstanceSize, sampleBufferAlignment);
        minAlignment = decoderAlignment > sampleBufferAlignment ? decoderAlignment : sampleBufferAlignment;
    }

    Decoder* pDecoder;
    System::GetInstance()->New2(&pDecoder, memRequired, minAlignment);

    if (!pDecoder)
    {
        return 0;
    }

    pDecoder->mpReleaseEvent = pDecoderDesc->pReleaseEvent;
    pDecoder->mSampleBufferStorage = NULL;
    pDecoder->SetChannels(numChannels);
    pDecoder->SetSystem(pSystem);

    if (!pDecoderDesc->pCreateInstanceEvent(pDecoder))
    {
        goto abort;
    }

    pDecoder->mpDecoder = pDecoder;
    pDecoder->mpDecodeEvent = pDecoderDesc->pDecodeEvent;
    pDecoder->mGuid = pDecoderDesc->guid;
    pDecoder->mInstanceSize = memRequired;
    pDecoder->mDecodedSamplesAvailable = 0;
    pDecoder->mDecodeSlotSamplesDecoded = 0;
    pDecoder->mFeedSlot = 0;
    pDecoder->mPrepareSlot = 0;
    pDecoder->mDecodeSlot = 0;
    pDecoder->mMaxSlots = static_cast<unsigned char>(maxRequests);
    pDecoder->mIsBlockBased = isBlockBased;
    uintptr_t pMemBlock;
    pMemBlock = reinterpret_cast<uintptr_t>(pDecoder) + decoderMemRequired;
    uintptr_t pRequestDescAddr;
    LinearAlloc(pRequestDescAddr, pMemBlock, requestDescMemRequired);
    pDecoder->mRequestDescOffset = static_cast<unsigned int>(pRequestDescAddr - reinterpret_cast<uintptr_t>(pDecoder));

    if (isBlockBased)
    {
        uintptr_t pSampleBuffer;
        unsigned int sampleBufferAlignment;
        unsigned int sampleBufferInstanceSize;
        sampleBufferInstanceSize = SampleBuffer::GetSize(numChannels, DEFAULT_SAMPLEBUFFER_MAXLOCKS, pDecoderDesc->maxBlockSize, &sampleBufferAlignment, pSystem);
        LinearAlloc(pSampleBuffer, pMemBlock, sampleBufferInstanceSize, sampleBufferAlignment);
        pDecoder->mSampleBufferOffset = static_cast<unsigned short>(pSampleBuffer - reinterpret_cast<uintptr_t>(pDecoder));
        unsigned int storageSize = SampleBuffer::CalculateStorageSize(numChannels, pDecoderDesc->maxBlockSize);
        pDecoder->mSampleBufferStorage = pSystem->Alloc(storageSize, 128);

        if (!pDecoder->mSampleBufferStorage)
        {
            goto abort;
        }

        SampleBuffer::CreateInstance(numChannels, DEFAULT_SAMPLEBUFFER_MAXLOCKS, pDecoderDesc->maxBlockSize, reinterpret_cast<void*>(pSampleBuffer), pDecoder->mSampleBufferStorage, pSystem);
    }

    pRequestDescArray = pDecoder->GetRequestDescArray();

    for (int i = 0; i < pDecoder->mMaxSlots; i++)
    {
        Decoder::RequestDesc* pRequestDesc = &pRequestDescArray[i];
        pRequestDesc->pSrc = 0;
        pRequestDesc->numSamples = 0;
    }

    return pDecoder;

abort:
    pDecoder->Release();

    return 0;
}
