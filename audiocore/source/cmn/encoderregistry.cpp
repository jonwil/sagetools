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

#include "encoderregistry.h"
#include "encoders\encpcm16big.h"
#include "encoders\encxas1.h"
#include "encoders\encealayer31.h"
#include "encoders\enclayer3.h"
#include "encoders\encealayer32.h"

void EncoderRegistry::RegisterAllEncoders()
{
    RegisterEncoder(Xas1Enc::GetEncoderDesc());
    RegisterEncoder(Pcm16BigEnc::GetEncoderDesc());
    RegisterEncoder(EaLayer3Enc::GetEncoderDesc());
    RegisterEncoder(EaLayer32PcmEnc::GetEncoderDesc());
    RegisterEncoder(EaLayer32SpikeEnc::GetEncoderDesc());
    RegisterEncoder(Layer3Enc::GetEncoderDesc());
}

EncoderRegistry* EncoderRegistry::CreateInstance(System* pSystem)
{
    EncoderRegistry* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        return pThis;
    }

    pThis->mpSystem = pSystem;
    return pThis;
}

void EncoderRegistry::Release()
{
    mpSystem->Delete(this);
}

EncoderRegistry::EncoderHandle EncoderRegistry::GetEncoderHandle(Guid guid)
{
    ListNode* pNext = mEncoderDescList.GetHead();

    while (pNext)
    {
        EncoderDesc* pEncoderDesc = GetEncoderDescFromNode(pNext);

        pNext = pNext->GetNext();

        if (pEncoderDesc->guid == guid)
        {
            return pEncoderDesc;
        }
    }

    return 0;
}

EncoderRegistry::EncoderHandle EncoderRegistry::RegisterEncoder(EncoderDesc* pEncoderDesc)
{
    ListNode* pNext = mEncoderDescList.GetHead();

    while (pNext)
    {
        EncoderDesc* pNode = GetEncoderDescFromNode(pNext);
        pNext = pNext->GetNext();

        if (pNode->guid == pEncoderDesc->guid)
        {
            return pNode;
        }
    }

    mEncoderDescList.Push(static_cast<ListNode*>(static_cast<void*>(&pEncoderDesc->listNode)));
    return pEncoderDesc;
}

Encoder* EncoderRegistry::EncoderFactory(EncoderHandle encoderHandle, int numChannels, int sampleRate, System* pSystem)
{
    EncoderDesc* pEncoderDesc = static_cast<EncoderDesc*>(encoderHandle);
    Encoder* pEncoder = pEncoderDesc->CreateInstance(numChannels, sampleRate, pSystem);

    if (!pEncoder)
    {
        return pEncoder;
    }

    pEncoder->SetSystem(pSystem);
    pEncoder->SetChannels(numChannels);
    pEncoder->SetSampleRate(sampleRate);
    pEncoder->SetEncoderDesc(pEncoderDesc);
    return pEncoder;
}
