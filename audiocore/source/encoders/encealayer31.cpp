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
#include "decoderregistry.h"
#include "decoders\decealayer31.h"
#include "encoders\mp3toea.h"
#include "system.h"
#include "endian.h"
#include "encoders\lame396\lame.h"
#include "encoders\lame396\lame_global_flags.h"
#include "encoders\encealayer31.h"
#include "cmn\layer3shared.h"

struct EALAYER3STRUCT
{
    class MP3toEA converter;
    void* pstate;
    struct MPEGAUDIOHDR mah;
    unsigned int phdr;
    unsigned char* pmp3buffer;
    unsigned int mp3bufsize;
    unsigned int bufferoffset;
    unsigned char isReset;
    unsigned char isFirstEncodeBlock;
    DecoderExtended* mpDecoder;
    unsigned char rawDataReady;
    int rawsamples;
    int rawOffset;
    int rawsamplesBytes;
    short* pRawIntData;
    int numChannels;
    int granuleCount;
    unsigned char* tempGranule;
    int curGranuleLen;
};

EncoderDesc EaLayer3Enc::sEncoderDesc = { GUID, CreateInstance, 0, 576 * 3, 0, true };

EncoderDesc* EaLayer3Enc::GetEncoderDesc()
{
    System* pSndSystem = System::GetInstance();
    DecoderRegistry* pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    pDecoderRegistry->RegisterDecoder(EaLayer31Dec::GetDecoderDesc());
    return &sEncoderDesc;
}

lame_global_flags* EaLayer3Enc::InitLAYER3(EALAYER3STRUCT* pLameStruct, int numChannels)
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

    pgf->VBR_q = (int)((100.0 - mVbrQuality * 100.0) * 9.0 / 100.0);
    pgf->VBR = vbr_mtrh;
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

Encoder* EaLayer3Enc::CreateInstance(int numChannels, int sampleRate, System* pSystem)
{
    unsigned int MAX_BYTES_EALAYER3FRAME = 6000;
    EaLayer3Enc* pThis;
    pSystem->New2(&pThis, 0);

    if (!pThis)
    {
        goto abort;
    }

    pThis->mNumEncoderInstances = (numChannels + 1) / 2;
    pThis->mpLameStructArray = (EALAYER3STRUCT*)pSystem->Alloc(pThis->mNumEncoderInstances * sizeof(EALAYER3STRUCT));
    memset(pThis->mpLameStructArray, 0x00, pThis->mNumEncoderInstances * sizeof(EALAYER3STRUCT));
    pThis->mppSourceDataPadding = (short*)pSystem->Alloc((pThis->mNumEncoderInstances * 2) * sizeof(short));
    pThis->mGranule2TempStorageSize = static_cast<int>(pThis->mNumEncoderInstances * MAX_BYTES_EALAYER3FRAME);
    pThis->mpGranule2TempStorage = (unsigned char*)pSystem->Alloc(static_cast<unsigned int>(pThis->mGranule2TempStorageSize));
    pThis->mGranule2Bytes = 0;
    pThis->mppSourceData = 0;
    pThis->mResidualSamples = 0;
    pThis->mIsInitialized = 0;
    pThis->mGranulesProduced = 0;
    pThis->mpBufferedEncodedData = 0;
    pThis->mBufferedEncodedDataBytes = 0;
    pThis->mpBufferedSeekData = 0;
    pThis->mBufferedSeekDataBytes = 0;
    pThis->mAverageDataRate = (float)(EALAYER3_AVERAGEDATARATE * numChannels);
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

void EaLayer3Enc::PreEncodeSetup(float* pSrc, int samples)
{
    lame_global_flags* pgf;
    EALAYER3STRUCT* pLameStruct;

    if (!mIsInitialized)
    {
        mIsInitialized = 1;
        pLameStruct = mpLameStructArray;
        int tmpChannels = GetChannels();

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            int encoderChannels;

            if ((tmpChannels > 1) && (tmpChannels > 2))
            {
                encoderChannels = 2;
            }
            else
            {
                encoderChannels = tmpChannels;
            }

            pgf = InitLAYER3(pLameStruct, encoderChannels);
            pLameStruct->converter.reset();
            pLameStruct->isReset = 1;
            pLameStruct->isFirstEncodeBlock = 1;
            pLameStruct->rawDataReady = 0;
            GetSystem()->Lock();
            DecoderRegistry* pDecoderRegistry = GetSystem()->GetDecoderRegistry();
            DecoderRegistry::DecoderHandle decoderHandle = pDecoderRegistry->GetDecoderHandle(EaLayer31Dec::GUID);
            pLameStruct->mpDecoder = pDecoderRegistry->DecoderExtendedFactory(decoderHandle, static_cast<unsigned int>(encoderChannels), 1, GetSystem());
            GetSystem()->Unlock();
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

int EaLayer3Enc::EncodeBlock(unsigned char* pDst, int numSamples, int* pBytesEncoded, int flush, void* pSeekData, int* pSeekDataBytes)
{
    int errorCode = 0;
    short* pRawPtr = 0;
    lame_global_flags* pgf;
    unsigned char* pStartingDst = pDst;
    EALAYER3STRUCT* pLameStruct = mpLameStructArray;
    int indexArray = 0;
    int framesEncoded = 0;
    *pBytesEncoded = 0;
    bool isSeekable = (pSeekData != 0);
    short* pCurrentSeekData = static_cast<short*>(pSeekData);

    if (pSeekDataBytes != NULL)
    {
        *pSeekDataBytes = 0;
    }

    for (int i = 0; i < mNumEncoderInstances; i++)
    {
        if (pLameStruct->isFirstEncodeBlock)
        {
            pLameStruct->rawsamples = (EALAYER3_RAW_BLOCKSAMPLES > numSamples) ? numSamples : EALAYER3_RAW_BLOCKSAMPLES;
            pLameStruct->numChannels = (mppSourceData[indexArray] == mppSourceData[indexArray + 1]) ? 1 : 2;
            pLameStruct->granuleCount = 0;
            pLameStruct->tempGranule = 0;
            pLameStruct->curGranuleLen = 0;
            pLameStruct->rawsamplesBytes = static_cast<int>(pLameStruct->rawsamples * sizeof(short) * pLameStruct->numChannels);
            pRawPtr = pLameStruct->pRawIntData = (short*)GetSystem()->Alloc(static_cast<unsigned int>(pLameStruct->rawsamplesBytes));

            for (int j = 0; j < pLameStruct->rawsamples; j++)
            {
                *pRawPtr++ = mppSourceData[indexArray][j];
            }

            if (pLameStruct->numChannels > 1)
            {
                for (int j = 0; j < pLameStruct->rawsamples; j++)
                {
                    *pRawPtr++ = mppSourceData[indexArray + 1][j];
                }
            }

            pLameStruct->rawDataReady = 1;
            pLameStruct->isFirstEncodeBlock = 0;
        }

        pgf = (lame_global_flags*)(pLameStruct->pstate);

        if (numSamples)
        {
            pgf->num_samples = static_cast<unsigned long>(numSamples);
            pLameStruct->bufferoffset += static_cast<unsigned int>(lame_encode_buffer(pgf, mppSourceData[indexArray], mppSourceData[indexArray + 1], numSamples, &pLameStruct->pmp3buffer[pLameStruct->bufferoffset], static_cast<int>((pLameStruct->mp3bufsize - pLameStruct->bufferoffset))));
        }
        else
        {
            int paddingSamples = (EALAYER3_GRANULESAMPLES * 2) - (mResidualSamples % (EALAYER3_GRANULESAMPLES * 2));
            short* pPadData0 = (short*)GetSystem()->Alloc(paddingSamples * sizeof(short));
            short* pPadData1 = pPadData0;

            for (int padCount = 0; padCount < paddingSamples; padCount++)
            {
                pPadData0[padCount] = mppSourceDataPadding[indexArray];
            }

            if (!(pLameStruct->numChannels % 2))
            {
                pPadData1 = (short*)GetSystem()->Alloc(paddingSamples * sizeof(short));

                for (int padCount = 0; padCount < paddingSamples; padCount++)
                {
                    pPadData1[padCount] = mppSourceDataPadding[indexArray + 1];
                }
            }

            pgf->num_samples = static_cast<unsigned long>(paddingSamples);

            pLameStruct->bufferoffset += static_cast<unsigned int>(lame_encode_buffer(pgf, pPadData0, pPadData1, paddingSamples, &pLameStruct->pmp3buffer[pLameStruct->bufferoffset], static_cast<int>(pLameStruct->mp3bufsize - pLameStruct->bufferoffset)));
            GetSystem()->Free(pPadData0);

            if (!(pLameStruct->numChannels % 2))
            {
                GetSystem()->Free(pPadData1);
            }
        }

        if (flush)
        {
            pLameStruct->bufferoffset += lame_encode_finish(pgf, &pLameStruct->pmp3buffer[pLameStruct->bufferoffset], static_cast<int>(pLameStruct->mp3bufsize));
        }

        pLameStruct++;
        indexArray += 2;
    }

    int continueParsing = 1;
    bool trueCondition = true;

    do
    {
        int bytesEncodedBeforePacket = *pBytesEncoded;
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
        int numGranules = 0;
        unsigned char* pGranule2TempStorage = mpGranule2TempStorage;
        mGranule2Bytes = 0;

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            errorCode = pLameStruct->converter.parse(pLameStruct->pmp3buffer);
            numGranules = pLameStruct->converter.getgranulecount();

            for (int j = 0; j < numGranules; j++)
            {
                int granule_len;
                unsigned char* pGranule;
                pLameStruct->converter.getgranule(j, pGranule, granule_len);
                unsigned char specialBlockTag;

                if ((pLameStruct->rawDataReady) && (pLameStruct->granuleCount >= 1))
                {
                    specialBlockTag = 0xEE;
                }
                else
                {
                    specialBlockTag = 0x00;
                }

                if (j == 0)
                {
                    *pDst++ = specialBlockTag;
                    *pBytesEncoded += 1;
                }
                else
                {
                    *pGranule2TempStorage++ = specialBlockTag;
                    mGranule2Bytes++;
                }

                if (j == 0)
                {
                    memcpy(pDst, pGranule, static_cast<size_t>(granule_len));
                    pDst += granule_len;
                    *pBytesEncoded += granule_len;
                }
                else
                {
                    memcpy(pGranule2TempStorage, pGranule, static_cast<size_t>(granule_len));
                    pGranule2TempStorage += granule_len;
                    mGranule2Bytes += granule_len;
                }

                pLameStruct->granuleCount++;

                if (pLameStruct->rawDataReady)
                {
                    if (pLameStruct->granuleCount <= 1)
                    {
                        if (!pLameStruct->tempGranule)
                        {
                            pLameStruct->tempGranule = (unsigned char*)GetSystem()->Alloc(2 * 10000 * sizeof(unsigned char));
                        }

                        pLameStruct->tempGranule[pLameStruct->curGranuleLen] = 0;
                        memcpy(&pLameStruct->tempGranule[pLameStruct->curGranuleLen + 1], pGranule, static_cast<size_t>(granule_len));
                        pLameStruct->curGranuleLen += granule_len + 1;
                    }
                    else
                    {
                        int samplesToDecode = pLameStruct->rawsamples;
                        float* tempDecodedData = (float*)GetSystem()->Alloc(2 * samplesToDecode * sizeof(float));
                        pLameStruct->tempGranule[pLameStruct->curGranuleLen] = 0;
                        memcpy(&pLameStruct->tempGranule[pLameStruct->curGranuleLen + 1], pGranule, static_cast<size_t>(granule_len));
                        Decoder::FeedType feedType;

                        if (pLameStruct->isReset)
                        {
                            feedType = Decoder::FEEDTYPE_NEW;
                            pLameStruct->isReset = 0;
                        }
                        else
                        {
                            feedType = Decoder::FEEDTYPE_CONTINUE;
                        }

                        pLameStruct->mpDecoder->Feed(pLameStruct->tempGranule, samplesToDecode, feedType);
                        pLameStruct->mpDecoder->Decode(tempDecodedData, samplesToDecode);

                        for (int k = 0; k < samplesToDecode * pLameStruct->numChannels; k++)
                        {
                            tempDecodedData[k] *= 32768.0f;
                        }

                        GetSystem()->Free(pLameStruct->tempGranule);
                        pLameStruct->tempGranule = 0;
                        pLameStruct->curGranuleLen = 0;
                        float fadeRate = 1.0f / (pLameStruct->rawsamples - 1);
                        float fade = 0.0f;
                        float* pDecodedData = tempDecodedData;
                        pRawPtr = pLameStruct->pRawIntData;
                        short* pRawPtr2 = 0;

                        if (pLameStruct->numChannels > 1)
                        {
                            pRawPtr2 = pLameStruct->pRawIntData + pLameStruct->rawsamples;
                        }

                        for (int k = 0; k < pLameStruct->rawsamples; k++)
                        {
                            short fadeResult = (short)((*pRawPtr * (1.0f - fade)) + (fade * (*pDecodedData++)));
                            ENDIAN::PutB(*pRawPtr++, fadeResult);

                            if (pLameStruct->numChannels > 1)
                            {
                                fadeResult = (short)((*pRawPtr2 * (1.0f - fade)) + (fade * (*pDecodedData++)));
                                ENDIAN::PutB(*pRawPtr2++, fadeResult);
                            }

                            fade += fadeRate;
                        }

                        GetSystem()->Free(tempDecodedData);
                        int* pDstTempInt;
                        unsigned char* pDstTempChar;

                        if (j == 0)
                        {
                            pDstTempInt = (int*)pDst;
                            pDstTempChar = pDst;
                        }
                        else
                        {
                            pDstTempInt = (int*)pGranule2TempStorage;
                            pDstTempChar = pGranule2TempStorage;
                        }

                        ENDIAN::PutB(*pDstTempInt++, pLameStruct->rawsamples);
                        pDstTempChar += sizeof(int);
                        ENDIAN::PutB(*pDstTempInt++, 0);
                        pDstTempChar += sizeof(int);
                        memcpy(pDstTempChar, pLameStruct->pRawIntData, static_cast<size_t>(pLameStruct->rawsamplesBytes));
                        pDstTempChar += pLameStruct->rawsamplesBytes;

                        if (j == 0)
                        {
                            *pBytesEncoded += (2 * sizeof(int)) + pLameStruct->rawsamplesBytes;
                            pDst += (2 * sizeof(int)) + pLameStruct->rawsamplesBytes;
                        }
                        else
                        {
                            mGranule2Bytes += ((2 * sizeof(int)) + pLameStruct->rawsamplesBytes);
                            pGranule2TempStorage += ((2 * sizeof(int)) + pLameStruct->rawsamplesBytes);
                        }

                        pLameStruct->rawDataReady = 0;
                        GetSystem()->Free(pLameStruct->pRawIntData);
                        pLameStruct->pRawIntData = 0;
                    }
                }
            }

            int bytesToMove = static_cast<int>(pLameStruct->bufferoffset - pLameStruct->mah.framebytes);
            memcpy(pLameStruct->pmp3buffer, &pLameStruct->pmp3buffer[pLameStruct->mah.framebytes], static_cast<size_t>(bytesToMove));
            pLameStruct->bufferoffset -= pLameStruct->mah.framebytes;
            pLameStruct->phdr = 0;
            pLameStruct++;

            if (i == mNumEncoderInstances - 1)
            {
                short packetLength = static_cast<short>(*pBytesEncoded - bytesEncodedBeforePacket);

                if (isSeekable)
                {
                    bytesEncodedBeforePacket = *pBytesEncoded;
                    ENDIAN::PutUB(*pCurrentSeekData++, packetLength);
                    *pSeekDataBytes += sizeof(short);
                }

                if (0 < mGranule2Bytes)
                {
                    memcpy(pDst, mpGranule2TempStorage, static_cast<size_t>(mGranule2Bytes));
                    pDst += mGranule2Bytes;
                    *pBytesEncoded += mGranule2Bytes;

                    if (isSeekable)
                    {
                        packetLength = static_cast<short>(*pBytesEncoded - bytesEncodedBeforePacket);
                        bytesEncodedBeforePacket = *pBytesEncoded;
                        ENDIAN::PutUB(*pCurrentSeekData++, packetLength);
                        *pSeekDataBytes += sizeof(short);
                    }
                }
            }
        }

        if ((mGranulesProduced + numGranules) == 1)
        {
            framesEncoded = 0;
        }
        else if ((mGranulesProduced + numGranules) == 2)
        {
            framesEncoded += (2 * EALAYER3_GRANULESAMPLES) - EALAYER3_LATENCY_FRAMES;
        }
        else
        {
            framesEncoded += numGranules * EALAYER3_GRANULESAMPLES;
        }

        mGranulesProduced += numGranules;

    } while (trueCondition);

    if ((framesEncoded <= 0) && (*pBytesEncoded > 0))
    {
        unsigned char* pTempEncodedData = 0;
        unsigned char* pTempSeekData = 0;

        if (mBufferedEncodedDataBytes > 0)
        {
            pTempEncodedData = (unsigned char*)GetSystem()->Alloc(static_cast<unsigned int>(mBufferedEncodedDataBytes + *pBytesEncoded));
            memcpy(pTempEncodedData, mpBufferedEncodedData, static_cast<size_t>(mBufferedEncodedDataBytes));
            GetSystem()->Free(mpBufferedEncodedData);
            mpBufferedEncodedData = pTempEncodedData;
            pTempEncodedData += mBufferedEncodedDataBytes;

            if (isSeekable)
            {
                pTempSeekData = static_cast<unsigned char*>(GetSystem()->Alloc(static_cast<unsigned int>(mBufferedSeekDataBytes + *pSeekDataBytes)));
                memcpy(pTempSeekData, mpBufferedSeekData, static_cast<size_t>(mBufferedSeekDataBytes));
                GetSystem()->Free(mpBufferedSeekData);
                mpBufferedSeekData = pTempSeekData;
                pTempSeekData += mBufferedSeekDataBytes;
            }
        }
        else
        {
            pTempEncodedData = (unsigned char*)GetSystem()->Alloc(static_cast<unsigned int>(*pBytesEncoded));
            mpBufferedEncodedData = pTempEncodedData;

            if (isSeekable)
            {
                pTempSeekData = static_cast<unsigned char*>(GetSystem()->Alloc(static_cast<unsigned int>(*pSeekDataBytes)));
                mpBufferedSeekData = pTempSeekData;
            }
        }

        memcpy(pTempEncodedData, pStartingDst, static_cast<size_t>(*pBytesEncoded));
        mBufferedEncodedDataBytes += *pBytesEncoded;
        *pBytesEncoded = 0;

        if (isSeekable)
        {
            memcpy(pTempSeekData, pSeekData, static_cast<size_t>(*pSeekDataBytes));
            mBufferedSeekDataBytes += *pSeekDataBytes;

            if (pSeekDataBytes != NULL)
            {
                *pSeekDataBytes = 0;
            }
        }
    }

    if ((framesEncoded > 0) || flush)
    {
        if (mBufferedEncodedDataBytes > 0)
        {
            unsigned char* pTempEncodedData = (unsigned char*)GetSystem()->Alloc(static_cast<size_t>(*pBytesEncoded));
            memcpy(pTempEncodedData, pStartingDst, static_cast<size_t>(*pBytesEncoded));
            memcpy(pStartingDst, mpBufferedEncodedData, static_cast<size_t>(mBufferedEncodedDataBytes));
            pStartingDst += mBufferedEncodedDataBytes;
            memcpy(pStartingDst, pTempEncodedData, static_cast<size_t>(*pBytesEncoded));
            GetSystem()->Free(pTempEncodedData);
            pTempEncodedData = 0;
            *pBytesEncoded += mBufferedEncodedDataBytes;
            mBufferedEncodedDataBytes = 0;
            GetSystem()->Free(mpBufferedEncodedData);
            mpBufferedEncodedData = 0;

            if (isSeekable)
            {
                unsigned char* pTempSeekData = static_cast<unsigned char*>(GetSystem()->Alloc(static_cast<size_t>(*pSeekDataBytes)));
                memcpy(pTempSeekData, pSeekData, static_cast<size_t>(*pSeekDataBytes));
                memcpy(pSeekData, mpBufferedSeekData, static_cast<size_t>(mBufferedSeekDataBytes));
                memcpy(static_cast<char*>(pSeekData) + mBufferedSeekDataBytes, pTempSeekData, static_cast<size_t>(*pSeekDataBytes));
                GetSystem()->Free(pTempSeekData);
                pTempSeekData = 0;
                *pSeekDataBytes += mBufferedSeekDataBytes;
                mBufferedSeekDataBytes = 0;
                GetSystem()->Free(mpBufferedSeekData);
                mpBufferedSeekData = 0;
            }
        }
    }

    return framesEncoded;
}

int EaLayer3Enc::Encode(float* pSrc, unsigned char* pDst, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    PreEncodeSetup(pSrc, numSamplesIn);
    int framesEncoded = 0;
    *pBytesEncoded = 0;
    mResidualSamples += numSamplesIn;
    framesEncoded = EncodeBlock(pDst, numSamplesIn, pBytesEncoded, 0, pSeekData, pSeekDataBytes);
    mResidualSamples -= framesEncoded;

    for (int i = 0; i < GetChannels(); i++)
    {
        GetSystem()->Free(mppSourceData[i]);
    }

    GetSystem()->Free(mppSourceData);
    mppSourceData = 0;
    return framesEncoded;
}

int EaLayer3Enc::Flush(unsigned char* pDst, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes)
{
    int framesFlushed = 0;
    *pBytesEncoded = 0;

    if (mIsInitialized)
    {
        EncodeBlock(pDst, 0, pBytesEncoded, 1, pSeekData, pSeekDataBytes);
        framesFlushed = mResidualSamples;
        mResidualSamples = 0;
        mGranulesProduced = 0;
    }

    int tmpChannels = GetChannels();
    lame_global_flags* pgf;
    EALAYER3STRUCT* pLameStruct = mpLameStructArray;

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
        pLameStruct->isFirstEncodeBlock = 1;
        pLameStruct->rawDataReady = 0;
        tmpChannels -= 2;
        pLameStruct++;
    }

    return framesFlushed;
}

void EaLayer3Enc::Release()
{
    if (mIsInitialized)
    {
        EALAYER3STRUCT* pLameStruct = mpLameStructArray;
        lame_global_flags* pgf;
        pgf = (lame_global_flags*)(pLameStruct->pstate);
        lame_close(pgf);

        for (int i = 0; i < mNumEncoderInstances; i++)
        {
            GetSystem()->Free(pLameStruct->pmp3buffer);
            pLameStruct->pmp3buffer = 0;
            pLameStruct->mpDecoder->Release();
            pLameStruct++;
        }
    }

    if (mpBufferedEncodedData)
    {
        GetSystem()->Free(mpBufferedEncodedData);
        mpBufferedEncodedData = 0;
        mBufferedEncodedDataBytes = 0;
    }

    if (mpBufferedSeekData)
    {
        GetSystem()->Free(mpBufferedSeekData);
        mpBufferedSeekData = 0;
        mBufferedSeekDataBytes = 0;
    }

    GetSystem()->Free(mppSourceDataPadding);
    GetSystem()->Free(mpGranule2TempStorage);
    GetSystem()->Free(mpLameStructArray);
    mpLameStructArray = 0;
    GetSystem()->Delete(this);
}

int EaLayer3Enc::GetSeekMemoryRequired(int numFrames)
{
    static const int SAMPLES_PER_GRANULE = 576;
    static const int EXTRA_PADDING = 8 * 1024;
    return static_cast<int>((numFrames / SAMPLES_PER_GRANULE + 2) * sizeof(short) + EXTRA_PADDING);
}
