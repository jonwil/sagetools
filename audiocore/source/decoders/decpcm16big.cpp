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

#include "decoders\decpcm16big.h"
#include "endian.h"

DecoderDesc Pcm16BigDec::sDecoderDesc = { GetSize, CreateInstanceEvent, 0, DecodeEvent, 0, GUID, 0, };

DecoderDesc* Pcm16BigDec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool Pcm16BigDec::CreateInstanceEvent(Decoder* pDecoder)
{
    Pcm16BigDec* pThis = static_cast<Pcm16BigDec*>(pDecoder);
    pThis->Reset();
    return true;
}

void Pcm16BigDec::Reset()
{
    mpSrc = 0;
    mRemainingSamples = 0;
}

inline void ConvertPCMToFloatUnVectorized(int startSample, int numSamples, short* pSrc, float* pDst, unsigned int stride)
{
    const float pcmToFloatConversionFactor = 0.000030517578125f;

    for (int sample = startSample; sample < numSamples; sample++)
    {
        short convertedSample;
        ENDIAN::PutB(convertedSample, *pSrc);
        pDst[sample] = static_cast<float>(convertedSample) * pcmToFloatConversionFactor;
        pSrc += stride;
    }
}

inline void ConvertPCMToFloatSSE(int numSamples, short* pSrc, float* pDst, unsigned int stride)
{
    __declspec(align(16)) int endianSwappedTemp[2];
    int numSamplesLeft = numSamples;
    unsigned int shortStride = stride * 2;
    static const __declspec(align(16)) float pcmToFloatConversionFactor[4] = { 0.000030517578125f,0.000030517578125f,0.000030517578125f,0.000030517578125f };

    _asm
    {
        mov esi, numSamplesLeft
        mov edi, [pDst]
        lea ecx, endianSwappedTemp
        mov edx, [pSrc]
        cmp esi, 7
        JLE $ExitLoop
        movaps xmm7, [pcmToFloatConversionFactor]
        $ConvertLoop:
        mov esi, shortStride
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx], eax
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx + 4], eax
        movq mm0, [ecx]
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx], eax
        psrad mm0, 16
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx + 4], eax
        cvtpi2ps xmm0, mm0
        movq mm1, [ecx]
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx], eax
        psrad mm1, 16
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx + 4], eax
        cvtpi2ps xmm1, mm1
        movq mm2, [ecx]
        mov ax, [edx]
        bswap eax
        add edx, esi
        mov[ecx], eax
        psrad mm2, 16
        mov ax, [edx]
        add edx, esi
        bswap eax
        mov[ecx + 4], eax
        mov esi, numSamplesLeft
        cvtpi2ps xmm2, mm2
        movq mm3, [ecx]
        psrad mm3, 16
        sub esi, 8
        shufps xmm0, xmm1, 044h
        cvtpi2ps xmm3, mm3
        mulps xmm0, xmm7
        shufps xmm2, xmm3, 044h
        movaps[edi], xmm0
        mulps xmm2, xmm7
        mov numSamplesLeft, esi
        movaps[edi + 16], xmm2
        add edi, 32
        cmp esi, 8
        jge $ConvertLoop
        $ExitLoop:
        emms
    }

    int startSample = numSamples - numSamplesLeft;
    pSrc += stride * startSample;
    ConvertPCMToFloatUnVectorized(startSample, numSamples, pSrc, pDst, stride);
}

int Pcm16BigDec::DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples)
{
    Pcm16BigDec* pThis = static_cast<Pcm16BigDec*>(pDecoder);

    if (pThis->mRemainingSamples <= 0)
    {
        RequestDesc* pRequestDesc = pThis->GetCurrentRequestDesc();

        if (pRequestDesc->feedType == Decoder::FEEDTYPE_NEW)
        {
            pThis->Reset();
        }

        pThis->mpSrc = reinterpret_cast<uintptr_t>(pRequestDesc->pSrc);
        pThis->mRemainingSamples = pRequestDesc->numSamples;

        if (pRequestDesc->decoderSkip)
        {
            pThis->mRemainingSamples -= pRequestDesc->decoderSkip;
            pThis->mpSrc += static_cast<int>(pRequestDesc->decoderSkip * pThis->GetChannels() * sizeof(short));
        }
    }

    for (unsigned int channel = 0; channel < pThis->GetChannels(); channel++)
    {
        short* pSrc = reinterpret_cast<short*>(pThis->mpSrc + (channel * sizeof(short)));
        float* pDst = pSampleBuffer->LockChannel(channel);

        if (staticDetectCPU.IsSSE())
        {
            ConvertPCMToFloatSSE(numSamples, pSrc, pDst, pThis->GetChannels());
        }
        else
        {
            ConvertPCMToFloatUnVectorized(0, numSamples, pSrc, pDst, pThis->GetChannels());
        }

        pSampleBuffer->UnlockChannel(channel);
    }

    pThis->mpSrc += static_cast<int>(numSamples * pThis->GetChannels() * sizeof(short));
    pThis->mRemainingSamples -= numSamples;
    return numSamples;
}
