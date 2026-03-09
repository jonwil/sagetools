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

#include "simex\simex.h"

float fcoeff[5][2] = { {0.0, 0.0}, {-0.9375, 0.0}, {-1.796875, 0.8125}, {-1.53125, 0.859375}, {-1.90625, 0.9375} };

int sencodebrrblock(short* psrc, unsigned char* pdst, int* s1, int* s2, double* d1, double* d2, int samplerep, int allowedfilters)
{
    double diff;
    int shift;
    int j;
    double minerror;
    short src[28];
    double tempfloat;
    int smp2;
    int smp1;
    short maxclip;
    double dstarr[5][28];
    double output;
    int dstq;
    short minclip;
    int tempint;
    double maxerror[5];
    int i;
    double sample;
    int filter;

    minerror = 1.0e21;
    smp1 = 0;
    smp2 = 0;
    filter = 0;

    if (samplerep == 10)
    {
        maxclip = 30000;
        minclip = -30000;
    }
    else
    {
        maxclip = 30719;
        minclip = -30720;
    }

    for (i = 0; i < 28; i++)
    {
        if (psrc[i] > maxclip)
        {
            src[i] = maxclip;
        }
        else if (psrc[i] < minclip)
        {
            src[i] = minclip;
        }
        else
        {
            src[i] = psrc[i];
        }
    }

    for (j = 0; j < 5; j++)
    {
        if ((allowedfilters & (1 << j)) != 0)
        {
            smp1 = *s1;
            smp2 = *s2;
            maxerror[j] = 0.0;

            for (i = 0; i < 28; i++)
            {
                sample = src[i];
                sample = smp1 * fcoeff[j][0] + smp2 * fcoeff[j][1] + sample;
                dstarr[j][i] = sample;
                tempfloat = sample > 0.0 ? sample : -sample;

                if (maxerror[j] < tempfloat)
                {
                    maxerror[j] = tempfloat;
                }

                smp2 = smp1;
                smp1 = src[i];
            }

            if (minerror > maxerror[j])
            {
                minerror = maxerror[j];
                filter = j;
            }

            if (!j && maxerror[0] <= 7.0)
            {
                filter = 0;
                break;
            }
        }
    }

    *s1 = smp1;
    *s2 = smp2;
    tempint = maxerror[filter];

    if (tempint > 0x7FFF)
    {
        tempint = 0x7FFF;
    }

    if (tempint < -32768)
    {
        tempint = -32768;
    }

    i = 0x4000;

    for (shift = 0; shift < 12; shift++, i >>= 1)
    {
        if ((i & (tempint + (i >> 3))) != 0)
        {
            break;
        }
    }

    *pdst = (filter * 16) & 0xF0 | shift & 0xF;

    if (samplerep == 12)
    {
        pdst[1] = *pdst;
        pdst++;
    }
    else if (samplerep == 5)
    {
        pdst[1] = 0;
        pdst++;
    }

    pdst = pdst + 1;

    for (i = 0; i < 28; i++)
    {
        output = dstarr[filter][i] + fcoeff[filter][0] * *d1 + fcoeff[filter][1] * *d2;
        tempint = (1 << shift) * output;
        dstq = (tempint + 2048) & 0xFFFFF000;

        if (dstq > 0x7FFF)
        {
            dstq = 0x7FFF;
        }

        if (dstq < -32768)
        {
            dstq = -32768;
        }

        if (samplerep == 10)
        {
            if ((i & 1) != 0)
            {
                *pdst++ |= (dstq >> 12) & 0xF;
            }
            else
            {
                *pdst = (dstq >> 8) & 0xF0;
            }
        }
        else if ((i & 1) != 0)
        {
            *pdst++ |= (dstq >> 8) & 0xF0;
        }
        else
        {
            *pdst = (dstq >> 12) & 0xF;
        }

        tempfloat = (double)(dstq >> shift);
        diff = tempfloat - output;
        *d2 = *d1;
        *d1 = diff;
    }

    return 1;
}

int sencodebrrblocks(short* psrc, unsigned char* pdst, int frames, int* s1, int* s2, double* d1, double* d2, int samplerep, int allowedfilters)
{
    int j;
    short src[28];
    int i;

    while (frames > 0)
    {
        if (frames < 28)
        {
            for (i = 0; i < frames; i++)
            {
                src[i] = psrc[i];
            }

            for (j = i - 1;i < 28;i++)
            {
                src[i] = psrc[j];
            }

            psrc = src;
        }

        sencodebrrblock(psrc, pdst, s1, s2, d1, d2, samplerep, allowedfilters);
        psrc += 28;
        pdst += 15;

        if (samplerep == 12 || samplerep == 5)
        {
            pdst++;
        }

        frames -= 28;
    }

    return 1;
}

int sencodexa(short* psrc, unsigned char* pdst, int frames, int* s1, int* s2, double* d1, double* d2, int allowedfilters)
{
    return sencodebrrblocks(psrc, pdst, frames, s1, s2, d1, d2, 10, allowedfilters & 0xF);
}
