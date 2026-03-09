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

#include "decoders\decpcm16little.h"
#include "endian.h"

DecoderDesc Pcm16LittleDec::sDecoderDesc = { GetSize, CreateInstanceEvent, 0, DecodeEvent, 0, GUID, 0 };

DecoderDesc* Pcm16LittleDec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool Pcm16LittleDec::CreateInstanceEvent(Decoder* pDecoder)
{
    Pcm16LittleDec* pThis = static_cast<Pcm16LittleDec*>(pDecoder);
    pThis->Reset();
    return true;
}

int Pcm16LittleDec::DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples)
{
    Pcm16LittleDec* pThis = static_cast<Pcm16LittleDec*>(pDecoder);

    if (pThis->mRemainingSamples <= 0)
    {
        RequestDesc* pRequestDesc = pThis->GetCurrentRequestDesc();

        if (pRequestDesc->feedType == Decoder::FEEDTYPE_NEW)
        {
            pThis->Reset();
        }

        pThis->mpSrc = reinterpret_cast<uintptr_t>(pRequestDesc->pSrc);
        pThis->mRemainingSamples = pRequestDesc->numSamples;
    }

    unsigned int numChannels = pThis->GetChannels();
    uintptr_t pLoadedSrc = pThis->mpSrc;

    for (unsigned int channel = 0; channel < numChannels; channel++)
    {
        short* pSrc = reinterpret_cast<short*>(pLoadedSrc + (channel * sizeof(short)));
        float* pDst = pSampleBuffer->LockChannel(channel);

        for (int sample = 0; sample < numSamples; sample++)
        {
            short convertedSample;
            ENDIAN::PutL(convertedSample, *pSrc);
            pDst[sample] = static_cast<float>(convertedSample) * (1.0f / 32767.0f);
            pSrc += numChannels;
        }

        pSampleBuffer->UnlockChannel(channel);
    }

    pThis->mpSrc += static_cast<int>(numSamples * numChannels * sizeof(short));
    pThis->mRemainingSamples -= numSamples;
    return numSamples;
}
