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
#include "cmn\isimex.h"
#include "cmn\fileio.h"
#include "cmn\cfftfloat.h"
#include <math.h>

struct FILTERDATASTRUCT
{
    int filtersize;
    int complementflag;
    int upsamplefactor;
    float resamplefactor;
    float globalgain;
    int numbreakpoints;
    float bpfreq[100];
    float bpgain[100];
};

static short convertsample(float x)
{
    short r;
    __asm
    {
        fld dword ptr x
        fistp word ptr r
    }

    if (r == -32768)
    {
        return 0x7fff + (*(unsigned int*)&x >> 31);
    }
    else
    {
        return r;
    }
}

inline int ifloorf(float x)
{
    int r;
    unsigned int d;

    __asm
    {
        fld dword ptr x
        fist dword ptr r
        fisub dword ptr r
        fstp dword ptr d
    }

    return r - (d >> 31);
}

static void initfilter(FILTERDATASTRUCT* filterdata, CFftFloat* fft, float* filter)
{
    int freqscale = 0x2000 / (2 * filterdata->upsamplefactor);
    float index1;
    float index2;
    float gain1;
    float gain2;
    int i;
    int j;
    int filtersize;
    float windowval;
    fft->mRealData[0] = filterdata->bpgain[0] * filterdata->globalgain;

    for (i = 1; i < filterdata->numbreakpoints; i++)
    {
        gain1 = filterdata->bpfreq[i + 99] * filterdata->globalgain;
        gain2 = filterdata->bpgain[i] * filterdata->globalgain;
        index1 = freqscale * filterdata->bpfreq[i - 1];
        index2 = freqscale * filterdata->bpfreq[i];
        
        for (j = ifloorf(index1) + 1; j <= ifloorf(index2); j++)
        {
            fft->mRealData[j] = (j - index1) * (gain2 - gain1) / (index2 - index1) + gain1;
        }
    }

    for (i = freqscale + 1; i < 0x2000 - freqscale; i++)
    {
        fft->mRealData[i] = 0.0;
    }

    for (i = 0x2000 - freqscale; i < 0x2000; i++)
    {
        fft->mRealData[i] = fft->mRealData[0x2000 - i];
    }

    for (i = 0; i < 0x2000; i++)
    {
        fft->mImagData[i] = 0.0;
    }

    fft->DoFft(13, 1);
    filtersize = filterdata->filtersize * filterdata->upsamplefactor;

    for (i = 1; i <= filtersize; i++)
    {
        windowval = cos(i * 3.14159265359f / filtersize) * 0.5 + 0.5;
        fft->mRealData[i] = fft->mRealData[i] * windowval;
        fft->mRealData[0x2000 - i] = fft->mRealData[0x2000 - i] * windowval;
    }

    for (i = filtersize + 1; i < 0x2000 - filtersize; i++)
    {
        fft->mRealData[i] = 0.0;
    }

    for (i = 0; i < 0x2000; i++)
    {
        fft->mImagData[i] = 0.0;
    }

    if (filterdata->complementflag)
    {
        fft->mRealData[0] = filterdata->globalgain - *fft->mRealData;

        for (i = 1; i <= filtersize; i++)
        {
            fft->mRealData[i] = -fft->mRealData[i];
            fft->mRealData[0x2000 - i] = -fft->mRealData[0x2000 - i];
        }
    }

    fft->DoFft(13, 0);

    for (i = 0; i < 0x2000; i++)
    {
        filter[i] = fft->mRealData[i];
    }
}

static void extractblock(short* rawdata, int length, int upsamplefactor, int startsample, float* data)
{
    int sample = startsample / upsamplefactor;
    int count = 0x2000 / upsamplefactor;
    int i;

    for (i = 0; i < count; i++)
    {
        if (sample < 0 || sample >= length)
        {
            data[i] = 0.0;
        }
        else
        {
            data[i] = (float)rawdata[sample] * upsamplefactor;
        }

        sample++;
    }
}

static void filterblocks(CFftFloat* fft, int upsamplefactor, float* filter)
{
    unsigned int order = 13;
    int i;

    for (i = upsamplefactor; i > 1; i >>= 1)
    {
        order--;
    }

    fft->DoFft(order, 0);
    int offset = 0;
    const int count = 0x2000 / upsamplefactor;
    int j;

    for (i = 1; i < upsamplefactor; i++)
    {
        offset += count;

        for (j = 0; j < count; j++)
        {
            fft->mRealData[offset + j] = fft->mRealData[j];
            fft->mImagData[offset + j] = fft->mImagData[j];
        }
    }

    for (i = 0; i < 0x2000; i++)
    {
        fft->mRealData[i] = fft->mRealData[i] * filter[i];
        fft->mImagData[i] = fft->mImagData[i] * filter[i];
    }

    fft->DoFft(13, 1);
}

static void computeslopes(float* data, int filtersize, float* slope)
{
    for (int i = filtersize; i < 0x2000 - filtersize; i++)
    {
        slope[i] = (data[i + 1] - data[i - 1]) * 0.5;
    }
}

static float interpolate(float* data, const float* slope, float x)
{
    int i1 = ifloorf(x);
    int i2 = i1 + 1;
    x -= i1;
    float a = slope[i1] + slope[i2] + 2.0 * (data[i1] - data[i2]);
    float b = data[i2] - data[i1] - slope[i1] - a;
    float c = slope[i1];
    float d = data[i1];
    return ((a * x + b) * x + c) * x + d;
}

static int getoutputsamplecount(int inputsamplecount, FILTERDATASTRUCT* filterdata)
{
    return (inputsamplecount * filterdata->upsamplefactor) * filterdata->resamplefactor;
}

static void resample(short* indata, int indatasize, FILTERDATASTRUCT* filterdata, short* outdata, int curchan, int totalchan)
{
    CFftFloat fft(13);
    int percentdone = 100 * curchan / totalchan;
    int percentperchan = 100 / totalchan;
    float* filter = (float*)Allocator::Alloc(32768);
    float* slope = (float*)Allocator::Alloc(32768);
    initfilter(filterdata, &fft, filter);
    int outdatasize = getoutputsamplecount(indatasize, filterdata);
    int upsamplefactor = filterdata->upsamplefactor;
    int filtersize = filterdata->filtersize * upsamplefactor;
    int blockstart = -filtersize;
    float* buffer;
    int blockstep = 0x2000 - 2 * filtersize - upsamplefactor;
    float x = blockstep;
    float xstep = 1.0 / filterdata->resamplefactor;
    buffer = fft.mImagData;
    int i;

    for (i = 0; i < outdatasize; i++)
    {
        if (x >= blockstep)
        {
            if (buffer == fft.mRealData)
            {
                buffer = fft.mImagData;
            }
            else
            {
                extractblock(indata, indatasize, upsamplefactor, blockstart, fft.mRealData);
                blockstart += blockstep;
                extractblock(indata, indatasize, upsamplefactor, blockstart, fft.mImagData);
                blockstart += blockstep;
                filterblocks(&fft, upsamplefactor, filter);
                buffer = fft.mRealData;
            }

            computeslopes(buffer, filtersize, slope);
            x -= blockstep;
            SIMEXI_progresscb(percentdone + i * percentperchan / outdatasize);
        }

        outdata[i] = convertsample(interpolate(buffer, slope, filtersize + x));
        x += xstep;
    }

    Allocator::Free(slope);
    Allocator::Free(filter);
}

int SIMEX_filterresample(SSOUND* pss, SIMEXFILTERPARAM* sfp)
{
    float ratio = (float)sfp->intval / (float)pss->samplerate;
    int i;
    FILTERDATASTRUCT fds;
    short* resampledtrack;
    MARKCHUNK* pmarkchunk;
    int j;

    if (pss->length * ratio <= 1.0)
    {
        pss->samplerate = sfp->intval;
        return 1;
    }

    if (pss->samplerate != sfp->intval)
    {
        fds.filtersize = 256;
        fds.complementflag = 0;
        fds.globalgain = 1.0;
        fds.bpfreq[0] = 0.0;
        fds.bpgain[0] = 1.0;

        if (sfp->intval > pss->samplerate)
        {
            fds.upsamplefactor = 2;
            fds.resamplefactor = ratio * 0.5;
            fds.bpfreq[1] = 0.95f;
            fds.bpgain[1] = 1.0;
            fds.bpfreq[2] = 1.0;
            fds.bpgain[2] = 0.0;
            fds.numbreakpoints = 3;
        }
        else
        {
            if (2 * sfp->intval < pss->samplerate)
            {
                fds.upsamplefactor = 1;
                fds.resamplefactor = ratio;
                fds.bpfreq[1] = ratio * 0.949999988079071;
                fds.bpgain[1] = 1.0;
                fds.bpfreq[2] = ratio;
                fds.bpgain[2] = 0.0;
                fds.bpfreq[3] = 1.0;
                fds.bpgain[3] = 0.0;
                fds.numbreakpoints = 4;
            }
            else
            {
                fds.upsamplefactor = 2;
                fds.resamplefactor = ratio * 0.5;
                fds.bpfreq[1] = ratio * 0.949999988079071;
                fds.bpgain[1] = 1.0;
                fds.bpfreq[2] = ratio;
                fds.bpgain[2] = 0.0;
                fds.bpfreq[3] = 1.0;
                fds.bpgain[3] = 0.0;
                fds.numbreakpoints = 4;
            }
        }

        for (i = 0; i < pss->numchannels; i++)
        {
            resampledtrack = (short*)Allocator::Alloc(2 * (int)(pss->length * ratio));

            if (resampledtrack)
            {
                resample(pss->track[i], pss->length, &fds, resampledtrack, i, pss->numchannels);
            }
            else
            {
                SIMEXI_setlasterr("Couldn't allocate memory needed for re-sampling.");
                return 0;
            }

            Allocator::Free(pss->track[i]);
            pss->track[i] = resampledtrack;
        }

        if (pss->sustainend > 0)
        {
            pss->sustainstart = (int)(pss->sustainstart * ratio + 0.5);
            pss->sustainend = (int)(pss->sustainend * ratio + 0.5);
        }

        for (i = 0; i < pss->markerchunkcount; i++)
        {
            pmarkchunk = pss->pmarkerchunklist[i];

            for (j = 0; j < pmarkchunk->nummarkers; j++)
            {
                if (pmarkchunk->marks[j])
                {
                    pmarkchunk->marks[j]->position = (int)(pmarkchunk->marks[j]->position * ratio + 0.5);
                }
            }
        }
        pss->length = (int)(pss->length * ratio);

        if (pss->sustainend >= pss->length)
        {
            pss->sustainend = pss->length - 1;
        }

        pss->samplerate = sfp->intval;
    }

    return 1;
}

void SIMEXI_filterregisterresample()
{
    SIMEXFILTERABOUT* psfa = &simexfilterabout[120];
    static SIMEXFILTERPARAMDESC sfpd[1];
    psfa->name = "Resample";
    psfa->cmdline = "rs";
    psfa->help = "Resample to a new rate.";
    psfa->numparams = 1;
    psfa->params = sfpd;
    psfa->beforeImport = 0;
    psfa->afterImport = 1;
    sfpd[0].name = "Sample Rate";
    sfpd[0].cmdline = "rate";
    sfpd[0].help = "Specified as new sample rate in Hertz.";
    sfpd[0].minval = 400.0;
    sfpd[0].maxval = 96000.0;
    sfpd[0].valtype = 0;
    simexfilter[120] = SIMEX_filterresample;
    simexfiltertypestrings[120] = "SIMEX_FILTER_RESAMPLE";
}
