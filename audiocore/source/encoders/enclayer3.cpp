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

#include <string.h>
#include "base.h"
#include "encoders\mp3toea.h"
#include "system.h"
#include "endian.h"
#include "encoders\lame396\lame.h"
#include "encoders\lame396\lame_global_flags.h"
#include "cmn\layer3shared.h"
#include "encoders\enclayer3.h"

struct LAYER3STRUCT
{
    MP3toEA converter;
    void* pstate;
    MPEGAUDIOHDR mah;
    unsigned int phdr;
    unsigned char* pmp3buffer;
    unsigned int mp3bufsize;
    unsigned int bufferoffset;
    unsigned char isReset;
};

EncoderDesc Layer3Enc::sEncoderDesc = { GUID, CreateInstance, 0, 0, 0, false };
EncoderDesc* Layer3Enc::GetEncoderDesc() { return &sEncoderDesc; }

int Layer3Enc::ChooseMpegBitRate(int tempBR, int samplerate)
{
    int mpegclass = 0;
    int i;
    int j;

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            if (samplerate == samplerate_table[i][j])
            {
                mpegclass = i;
                break;
            }
        }
    }

    for (i = 1; i < 15; i++)
    {
        if (bitrate_table[mpegclass][i] == tempBR)
        {
            return bitrate_table[mpegclass][i];
        }

        if (bitrate_table[mpegclass][i] > tempBR)
        {
            printf("Non-standard MPEG bit rate of %i000 detected.  Using %i000 instead...\n", tempBR, bitrate_table[mpegclass][i]);
            return bitrate_table[mpegclass][i];
        }
    }

    printf("Invalid bit rate / sampling rate pair.  Using highest bit rate %i000 instead.\n", bitrate_table[mpegclass][14]);
    return bitrate_table[mpegclass][14];
}

lame_global_flags* Layer3Enc::InitLAYER3(LAYER3STRUCT* pLameStruct, int numChannels)
{
    lame_global_flags* pgf;
    int retVal = (int)(pLameStruct->pstate = (pgf = lame_init()));
    pLameStruct->pstate = pgf;
    pLameStruct->phdr = 0;
    pLameStruct->bufferoffset = 0;

    if (numChannels == 2)
    {
        pgf->num_channels = 2;
    }
    else
    {
        pgf->num_channels = 1;
    }

    if (GetBitRateManagement() == BITRATEMANAGEMENT_USINGVBR)
    {
        pgf->VBR_q = (int)((100.0 - mVbrQuality * 100.0) * 9.0 / 100.0);
        pgf->VBR = vbr_mtrh;
    }
    else
    {
        int temp_bitrate = 0;
        pgf->VBR = vbr_off;

        if (pgf->num_channels <= 2)
        {
            temp_bitrate = mCbrRate / 1000;
        }
        else
        {
            temp_bitrate = mCbrRate / (1000 * pgf->num_channels);
        }

        pgf->brate = ChooseMpegBitRate(temp_bitrate, GetSampleRate());
        pgf->VBR_min_bitrate_kbps = pgf->brate;
    }

    pgf->in_samplerate = GetSampleRate();
    pgf->out_samplerate = GetSampleRate();

    if (pgf->num_channels == 2)
    {
        pgf->mode = JOINT_STEREO;
    }
    else
    {
        pgf->mode = MONO;
    }

    pgf->quality = 2;
    pgf->bWriteVbrTag = 0;
    pgf->lowpassfreq = 0;
    retVal = lame_init_params(pgf);
    pLameStruct->mah.version = pgf->version;
    return (lame_global_flags*)(pLameStruct->pstate);
}

Encoder* Layer3Enc::CreateInstance(int numChannels, int sampleRate, System* pSystem)
{
    Layer3Enc* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        goto abort;
    }

    pThis->mNumEncoderInstances = 1;
    pThis->mpLameStructArray = (LAYER3STRUCT*)pSystem->Alloc(pThis->mNumEncoderInstances * sizeof(LAYER3STRUCT));
    memset(pThis->mpLameStructArray, 0x00, pThis->mNumEncoderInstances * sizeof(LAYER3STRUCT));
    pThis->mppSourceDataPadding = (short*)pSystem->Alloc((pThis->mNumEncoderInstances * 2) * sizeof(short));
    pThis->mppSourceData = 0;
    pThis->mResidualSamples = 0;
    pThis->mIsInitialized = 0;
    pThis->mAverageDataRate = (float)(LAYER3_AVERAGEDATARATE * numChannels);
    pThis->mVbrQuality = 0.0f;
    pThis->mCbrRate = 4000;
    return static_cast<Encoder*>(pThis);

abort:
    if (pThis)
    {
        pSystem->Delete(pThis);
    }

    return 0;
}

void Layer3Enc::PreEncodeSetup(float* pSrc, int samples)
{
    lame_global_flags* pgf;
    LAYER3STRUCT* pLameStruct;

    if (!mIsInitialized)
    {
        mIsInitialized = 1;
        pLameStruct = mpLameStructArray;
        int tmpChannels = GetChannels();

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            if ((tmpChannels > 1) && (tmpChannels > 2))
            {
                pgf = InitLAYER3(pLameStruct, 2);
            }
            else
            {
                pgf = InitLAYER3(pLameStruct, tmpChannels);
            }

            pLameStruct->converter.reset();
            pLameStruct->isReset = 1;
            tmpChannels -= 2;
            pLameStruct++;
        }
    }

    pLameStruct = mpLameStructArray;
    unsigned int newbufsize = (unsigned int)(2.50 * (samples + 7200));

    for (int i = 0; i < mNumEncoderInstances; i++)
    {
        if (pLameStruct->mp3bufsize < (pLameStruct->bufferoffset + newbufsize))
        {
            pLameStruct->mp3bufsize = pLameStruct->bufferoffset + newbufsize;
            unsigned char* pTemp = (unsigned char*)GetSystem()->Alloc(pLameStruct->mp3bufsize);

            if (pLameStruct->pmp3buffer)
            {
                memcpy(pTemp, pLameStruct->pmp3buffer, pLameStruct->bufferoffset);
                GetSystem()->Free(pLameStruct->pmp3buffer);
            }

            pLameStruct->pmp3buffer = pTemp;
            pLameStruct++;
        }
    }

    int numTracks;

    if (GetChannels() % 2)
    {
        numTracks = GetChannels() + 1;
    }
    else
    {
        numTracks = GetChannels();
    }

    mppSourceData = (short**)GetSystem()->Alloc(numTracks * sizeof(short*));

    for (int i = 0; i < GetChannels(); i++)
    {
        mppSourceData[i] = (short*)GetSystem()->Alloc(samples * sizeof(short));
    }

    if (GetChannels() % 2)
    {
        mppSourceData[GetChannels()] = mppSourceData[GetChannels() - 1];
    }

    for (int j = 0; j < samples; j++)
    {
        for (int i = 0; i < GetChannels(); i++)
        {
            if (*pSrc > 1.0f)
            {
                mppSourceData[i][j] = 32767;
            }
            else if (*pSrc < -1.0f)
            {
                mppSourceData[i][j] = -32768;
            }
            else
            {
                mppSourceData[i][j] = (short)(*pSrc * 32768.0f);
            }

            pSrc++;
        }
    }

    for (int i = 0; i < GetChannels(); i++)
    {
        mppSourceDataPadding[i] = mppSourceData[i][samples - 1];
    }
}

int Layer3Enc::EncodeBlock(unsigned char* pDst, int numSamples, int* bytesEncoded, int flush)
{
    int errorCode = 0;
    lame_global_flags* pgf;
    LAYER3STRUCT* pLameStruct = mpLameStructArray;
    int indexArray = 0;
    int framesEncoded = 0;
    *bytesEncoded = 0;

    for (int i = 0; i < mNumEncoderInstances; i++)
    {
        if (pLameStruct->isReset)
        {
            pLameStruct->isReset = 0;
        }

        pgf = (lame_global_flags*)(pLameStruct->pstate);

        if (numSamples)
        {
            pgf->num_samples = static_cast<unsigned long>(numSamples);
            pLameStruct->bufferoffset += static_cast<unsigned int>(lame_encode_buffer(pgf, mppSourceData[indexArray], mppSourceData[indexArray + 1], numSamples, &pLameStruct->pmp3buffer[pLameStruct->bufferoffset], static_cast<int>(pLameStruct->mp3bufsize - pLameStruct->bufferoffset)));
        }

        if (flush)
        {
            pLameStruct->bufferoffset += static_cast<unsigned int>(lame_encode_finish(pgf, &pLameStruct->pmp3buffer[pLameStruct->bufferoffset], static_cast<int>(pLameStruct->mp3bufsize)));
        }

        pLameStruct++;
        indexArray += 2;
    }

    int continueParsing = 1;
    bool trueCondition = true;

    do
    {
        pLameStruct = mpLameStructArray;

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            if (pLameStruct->bufferoffset == 0)
            {
                continueParsing = 0;
            }
            else
            {
                if (pLameStruct->phdr == 0)
                {
                    pLameStruct->phdr = static_cast<unsigned int>((pLameStruct->pmp3buffer[0] << 24) + (pLameStruct->pmp3buffer[1] << 16) + (pLameStruct->pmp3buffer[2] << 8) + pLameStruct->pmp3buffer[3]);
                    errorCode = ParseMP3header(pLameStruct->phdr, &pLameStruct->mah);
                }

                if (pLameStruct->bufferoffset < pLameStruct->mah.framebytes)
                {
                    continueParsing = 0;
                }
            }

            pLameStruct++;
        }

        if (!continueParsing)
        {
            break;
        }

        pLameStruct = mpLameStructArray;

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            memcpy(pDst, pLameStruct->pmp3buffer, pLameStruct->mah.framebytes);
            pDst += pLameStruct->mah.framebytes;
            *bytesEncoded += pLameStruct->mah.framebytes;
            int bytesToMove = static_cast<int>(pLameStruct->bufferoffset - pLameStruct->mah.framebytes);
            memcpy(pLameStruct->pmp3buffer, &pLameStruct->pmp3buffer[pLameStruct->mah.framebytes], static_cast<size_t>(bytesToMove));
            pLameStruct->bufferoffset -= pLameStruct->mah.framebytes;
            pLameStruct->phdr = 0;
            framesEncoded += pLameStruct->mah.numframes;
            pLameStruct++;
        }
    } while (trueCondition);

    return framesEncoded;
}

int Layer3Enc::Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    PreEncodeSetup(pSrc, numSamplesIn);
    int framesEncoded = 0;
    *pBytesEncoded = 0;
    mResidualSamples += numSamplesIn;
    framesEncoded = EncodeBlock(pDst, numSamplesIn, pBytesEncoded, 0);
    mResidualSamples -= framesEncoded;

    for (int i = 0; i < GetChannels(); i++)
    {
        GetSystem()->Free(mppSourceData[i]);
    }

    GetSystem()->Free(mppSourceData);
    return framesEncoded;
}

int Layer3Enc::Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    if (pSeekDataBytes)
    {
        *pSeekDataBytes = 0;
    }

    int framesFlushed = 0;
    *pBytesEncoded = 0;

    if (mIsInitialized)
    {
        EncodeBlock(pDst, 0, pBytesEncoded, 1);
        framesFlushed = mResidualSamples;
        mResidualSamples = 0;
    }

    int tmpChannels = GetChannels();
    lame_global_flags* pgf;
    LAYER3STRUCT* pLameStruct = mpLameStructArray;

    for (int i = 0; i < mNumEncoderInstances; i++)
    {
        if ((tmpChannels > 1) && (tmpChannels > 2))
        {
            pgf = InitLAYER3(pLameStruct, 2);
        }
        else
        {
            pgf = InitLAYER3(pLameStruct, tmpChannels);
        }

        pLameStruct->converter.reset();
        pLameStruct->isReset = 1;
        tmpChannels -= 2;
        pLameStruct++;
    }

    return framesFlushed;
}

void Layer3Enc::Release()
{
    if (mIsInitialized)
    {
        LAYER3STRUCT* pLameStruct = mpLameStructArray;
        lame_global_flags* pgf;
        pgf = (lame_global_flags*)(pLameStruct->pstate);
        lame_close(pgf);

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            GetSystem()->Free(pLameStruct->pmp3buffer);
            pLameStruct->pmp3buffer = 0;
            pLameStruct++;
        }
    }

    GetSystem()->Free(mppSourceDataPadding);
    GetSystem()->Free(mpLameStructArray);
    mpLameStructArray = 0;
    GetSystem()->Delete(this);
}
