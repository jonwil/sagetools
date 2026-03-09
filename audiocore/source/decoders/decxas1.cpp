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

#include "decoders\decxas1.h"

DecoderDesc Xas1Dec::sDecoderDesc = { GetSize, CreateInstanceEvent, 0, DecodeEvent, 0, GUID, 128 };

DecoderDesc* Xas1Dec::GetDecoderDesc()
{
    return &sDecoderDesc;
}

bool Xas1Dec::CreateInstanceEvent(Decoder* pDecoder)
{
    Xas1Dec* pThis = static_cast<Xas1Dec*>(pDecoder);
    pThis->Reset();
    return true;
}

__declspec(align(16)) const float Xas1Dec::sFilterPairs[4][2] =
{
    { 0.0f, 0.0f },
    { 0.9375f, 0.0f },
    { 1.796875f, -0.8125f },
    { 1.53125f, -0.859375f }
};

const float Xas1Dec::sShiftMulLut[13] =
{
    4.65661287307739260000e-010,
    2.32830643653869630000e-010,
    1.16415321826934810000e-010,
    5.82076609134674070000e-011,
    2.91038304567337040000e-011,
    1.45519152283668520000e-011,
    7.27595761418342590000e-012,
    3.63797880709171300000e-012,
    1.81898940354585650000e-012,
    9.09494701772928240000e-013,
    4.54747350886464120000e-013,
    2.27373675443232060000e-013,
    1.13686837721616030000e-013
};

__forceinline void Decode30Samples(unsigned char* pSrc, float* pDst, float shiftMul[], float f0[], float f1[])
{
    for (int i = 0; i < 15; i++)
    {
        for (int block = 0; block < 4; block++)
        {
            int s0 = pSrc[block] >> 4;
            int s1 = pSrc[block] << 28;
            s0 <<= 28;
            float s0Shifted = s0 * shiftMul[block];
            float s1Shifted = s1 * shiftMul[block];
            float* pTempDst = pDst + block * 32;
            pTempDst[0] = s0Shifted + (f0[block] * pTempDst[-1]) + (f1[block] * pTempDst[-2]);
            pTempDst[1] = s1Shifted + (f0[block] * pTempDst[0]) + (f1[block] * pTempDst[-1]);
        }

        pDst += 2;
        pSrc += 4;
    }
}

int Xas1Dec::DecodeEvent(Decoder* pDecoder, SampleBuffer* pSampleBuffer, int numSamples)
{
    Xas1Dec* pThis = static_cast<Xas1Dec*>(pDecoder);
    int skipRemaining = 0;

    if (pThis->mRemainingSamples <= 0)
    {
        RequestDesc* pRequestDesc = pThis->GetCurrentRequestDesc();

        if (pRequestDesc->feedType == Decoder::FEEDTYPE_NEW)
        {
            pThis->Reset();
        }

        pThis->mpEncodedSample = static_cast<unsigned char*>(pRequestDesc->pSrc);
        int groupsSkipped = pRequestDesc->decoderSkip / SAMPLES_PER_GROUP;
        int bytesSkipped = groupsSkipped * BYTES_PER_GROUP * static_cast<int>(pThis->GetChannels());
        pThis->mpEncodedSample += bytesSkipped;
        skipRemaining = pRequestDesc->decoderSkip - groupsSkipped * SAMPLES_PER_GROUP;
        pThis->mRemainingSamples = pRequestDesc->numSamples - pRequestDesc->decoderSkip;
    }

    unsigned int numChannels = pThis->GetChannels();

    for (unsigned int chan = 0; chan < numChannels; chan++)
    {
        unsigned char* pSrc = pThis->mpEncodedSample;
        float* pDstBegin = pSampleBuffer->LockChannel(chan);
        float* pDst = pDstBegin;
        pThis->DecodeChannel(pSrc, pDst);
        pThis->mpEncodedSample += 76;

        if (0 < skipRemaining)
        {
            int usableSamples = SAMPLES_PER_GROUP - skipRemaining;
            memmove(pDstBegin, pDstBegin + skipRemaining, usableSamples * sizeof(float));
        }

        pSampleBuffer->UnlockChannel(chan);
    }

    int samplesProduced = SAMPLES_PER_GROUP;

    if (0 < skipRemaining)
    {
        samplesProduced -= skipRemaining;
    }

    pThis->mRemainingSamples -= samplesProduced;
    return samplesProduced;
}

void Xas1Dec::DecodeChannel(unsigned char* pSrc, float* pDst)
{
    __declspec(align(16)) float f0[BLOCKS_PER_GROUP];
    __declspec(align(16)) float f1[BLOCKS_PER_GROUP];
    __declspec(align(16)) float shiftMul[BLOCKS_PER_GROUP];
    __declspec(align(16)) float interleavedDst[2 * BLOCKS_PER_GROUP];
    float* pInterleavedDst = interleavedDst;

    for (int block = 0; block < BLOCKS_PER_GROUP; block++)
    {
        int filt = pSrc[0] & 0x0f;
        f0[block] = sFilterPairs[filt][0];
        f1[block] = sFilterPairs[filt][1];
        pInterleavedDst[block] = static_cast<float>((pSrc[0] & 0xf0) + (static_cast<signed char>(pSrc[1]) << 8)) * (1.0f / 32768.0f);
        pDst[block * SAMPLES_PER_BLOCK] = pInterleavedDst[block];
        int shift = pSrc[2] & 0x0f;
        shiftMul[block] = sShiftMulLut[shift];
        pInterleavedDst[block + BLOCKS_PER_GROUP] = static_cast<float>((pSrc[2] & 0xf0) + (static_cast<signed char>(pSrc[3]) << 8)) * (1.0f / 32768.0f);
        pDst[block * SAMPLES_PER_BLOCK + 1] = pInterleavedDst[block + BLOCKS_PER_GROUP];
        pSrc += 4;
    }

    pDst += 2;

    if (staticDetectCPU.IsSSE())
    {
        __declspec(align(16)) int highNibble[2] = { 0xf0000000, 0xf0000000 };

        __asm
        {
            push edx
            push esi
            push edi
            mov esi, pSrc
            mov edi, pInterleavedDst
            mov edx, esi
            movaps xmm7, [edi]
            movaps xmm6, [edi + 16]
            add edx, 4 * 15
            mov edi, pDst
            $DecodeBlockLoop:
            pxor mm0, mm0
            punpcklbw mm0, [esi]
            pxor mm1, mm1
            movq mm3, mm0
            pxor mm2, mm2
            punpcklwd mm1, mm0
            punpckhwd mm2, mm3
            movq mm3, mm1
            movq mm0, mm2
            pslld mm0, 4
            pslld mm1, 4
            pand mm2, [highNibble]
            pand mm3, [highNibble]
            cvtpi2ps xmm0, mm2
            cvtpi2ps xmm1, mm0
            movlhps xmm0, xmm0
            movlhps xmm1, xmm1
            cvtpi2ps xmm0, mm3
            cvtpi2ps xmm1, mm1
            movaps xmm5, xmm6
            mulps xmm0, [shiftMul]
            mulps xmm5, [f0]
            mulps xmm7, [f1]
            addps xmm0, xmm5
            mulps xmm1, [shiftMul]
            addps xmm7, xmm0
            mulps xmm6, [f1]
            movaps xmm5, xmm7
            mulps xmm5, [f0]
            addps xmm1, xmm5
            addps xmm6, xmm1
            movaps xmm5, xmm7
            movaps xmm4, xmm6
            movaps xmm3, xmm7
            unpckhps xmm5, xmm6
            unpcklps xmm3, xmm4
            movlps[edi + 2 * 128], xmm5
            movhps[edi + 3 * 128], xmm5
            movlps[edi], xmm3
            movhps[edi + 128], xmm3
            add edi, 8
            add esi, 4
            cmp esi, edx
            jne $DecodeBlockLoop
            emms
            pop edi
            pop esi
            pop edx
        }
    }
    else
    {
        Decode30Samples(pSrc, pDst, shiftMul, f0, f1);
    }
}
