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

#include "scalesamples.h"
#include "base.h"

__forceinline void ScaleSamplesSseImplementation(float* pData, float gain, int numSamples)
{
    float* pEndDst = pData + numSamples;

    __asm
    {
        mov esi, [pData]
        mov edx, [pEndDst]
        movss xmm7, [gain]
        movlhps xmm7, xmm7
        shufps xmm7, xmm7, 8
        $copywithgain_sse_main_loop:
        movaps xmm0, [esi]
        movaps xmm1, [esi + 16]
        mulps xmm0, xmm7
        mulps xmm1, xmm7
        movaps[esi], xmm0; *pData *= gain
        movaps[esi + 16], xmm1; *pData *= gain
        movaps xmm2, [esi + 32]
        movaps xmm3, [esi + 48]
        mulps xmm2, xmm7
        mulps xmm3, xmm7
        movaps[esi + 32], xmm2; *pData *= gain
        movaps[esi + 48], xmm3; *pData *= gain
        add esi, 64
        cmp esi, edx
        jne $copywithgain_sse_main_loop
    }
}

__forceinline void ScaleSamplesOptimizedImplementation(float* pData, float gain, int numSamples)
{
    float* pEndDst = pData + numSamples;

    while (pData < pEndDst)
    {
        pData[0] *= gain;
        pData[1] *= gain;
        pData[2] *= gain;
        pData[3] *= gain;
        pData += 4;
    }
}

__forceinline void ScaleSamplesImplementation(float* pData, float gain, int numSamples)
{
    float* pEndDst = pData + numSamples;

    while (pData < pEndDst)
    {
        *pData *= gain;
        pData++;
    }
}

void ScaleSamples(float* pData, float gain, int numSamples)
{
    if (numSamples > 0)
    {
        if (((reinterpret_cast<uintptr_t>(pData) & 0x0f) == 0) && ((numSamples & 0x0f) == 0))
        {
            if (staticDetectCPU.IsSSE())
            {
                ScaleSamplesSseImplementation(pData, gain, numSamples);
            }
            else
            {
                ScaleSamplesOptimizedImplementation(pData, gain, numSamples);
            }
        }
        else
        {
            ScaleSamplesImplementation(pData, gain, numSamples);
        }
    }
}
