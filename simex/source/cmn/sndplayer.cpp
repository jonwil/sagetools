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

#include "bitgetter.h"
#include "cmn\fileio.h"
#include "cmn\PutBits.h"
#include "endian.h"
#include "cmn\encoderhelper.h"
#include "cmn\isimex.h"
#include "decoderregistry.h"
#include "coda\extern\ftoi.h"

struct SndPlayerHeaderInfo
{
    char* pSampleData;
    unsigned int sampleRate;
    int numSamples;
    int loopStart;
    int loopStartStreamOffset;
    int gigaSamplesInRam;
    unsigned char version;
    unsigned char codec;
    unsigned char numChannels;
    unsigned char playType;
};

void AddExtToSndPlayerFile(char* newdstfilename, char* orgdstfilename, const char* rule, const char* ext)
{
    strcpy(newdstfilename, orgdstfilename);

    if (rule)
    {
        strcat(newdstfilename, rule);
    }

    strcat(newdstfilename, ".");
    strcat(newdstfilename, ext);
}

void ChangeSndPlayerFileExtension(char* newdstfilename, const char* orgdstfilename, const char* pExt)
{
    char* pNoExtFilename;
    char workspace[1024];

    strcpy(workspace, orgdstfilename);
    pNoExtFilename = _strrev(workspace) + 4;
    AddExtToSndPlayerFile(newdstfilename, _strrev(pNoExtFilename), 0, pExt);
}

int ReadSndPlayerFileHeader(SndPlayerHeaderInfo* pInfo, char* pPackedHeader)
{
    BitGetter bitGetter;
    bitGetter.SetBitBuffer(pPackedHeader);
    pInfo->version = bitGetter.GetBits(4);

    if (pInfo->version)
    {
        return -1;
    }

    pInfo->codec = bitGetter.GetBits(4);

    if (pInfo->codec != 4 && pInfo->codec != 0 && pInfo->codec != 1 && pInfo->codec != 5 && pInfo->codec != 3 && pInfo->codec != 2 && pInfo->codec != 6 && pInfo->codec != 7)
    {
        return -1;
    }

    pInfo->numChannels = bitGetter.GetBits(6) + 1;

    if (pInfo->numChannels <= 0 || pInfo->numChannels > 64)
    {
        return -1;
    }

    pInfo->sampleRate = bitGetter.GetBits(18);

    if (pInfo->sampleRate <= 0 && pInfo->sampleRate < 200000)
    {
        return -1;
    }

    pInfo->playType = bitGetter.GetBits(2);

    if (pInfo->playType != 0 && pInfo->playType != 1 && pInfo->playType != 2)
    {
        return -1;
    }

    int loopFlag = bitGetter.GetBits(1);
    pInfo->numSamples = bitGetter.GetBits(29);

    if (loopFlag)
    {
        pInfo->loopStart = bitGetter.GetBits(32);

        if (pInfo->loopStart < 0 || pInfo->loopStart >= pInfo->numSamples)
        {
            return -1;
        }
    }
    else
    {
        pInfo->loopStart = -1;
    }

    if (pInfo->playType == 2)
    {
        pInfo->gigaSamplesInRam = bitGetter.GetBits(32);
    }

    if (loopFlag)
    {
        if (pInfo->playType == 1 || pInfo->playType == 2 && pInfo->loopStart >= pInfo->gigaSamplesInRam)
        {
            pInfo->loopStartStreamOffset = bitGetter.GetBits(32);

            if (pInfo->loopStartStreamOffset < 0 || pInfo->loopStartStreamOffset >= 0x40000000)
            {
                return -1;
            }
        }
        else
        {
            pInfo->loopStartStreamOffset = 0;
        }
    }

    pInfo->pSampleData = &pPackedHeader[bitGetter.GetBitPosition() >> 3];
    return 0;
}

int ImportReadSndPlayerHelper(SndPlayerHeaderInfo headerInfo, DecoderExtended* pDecoder, int curSample, char* pDataPointer, int sampleDataSize, short* ptracks[])
{
    int decodedSamples = 0;
    int totalDecodedSamples = 0;
    char* pDataBegin = pDataPointer;
    char* pDataEnd = &pDataPointer[sampleDataSize];
    short* tempTracks[64];
    int repeatLoop = 1;

    while (pDataBegin < pDataEnd)
    {
        for (int i = 0; i < headerInfo.numChannels; i++)
        {
            tempTracks[i] = &ptracks[i][totalDecodedSamples];
        }

        unsigned int* pChunk = (unsigned int*)pDataBegin;
        unsigned int chunkBytes;
        unsigned int chunkSamples;
        PutM(&chunkBytes, *pChunk++, 4);
        PutM(&chunkSamples, *pChunk++, 4);
        char* pChunkData = (char*)pChunk;
        chunkBytes &= ~0x80000000;

        if (!chunkBytes)
        {
            repeatLoop = 0;
            break;
        }

        Decoder::FeedType feedType;

        if (!curSample || curSample == headerInfo.loopStart)
        {
            feedType = Decoder::FEEDTYPE_NEW;
        }
        else
        {
            feedType = Decoder::FEEDTYPE_CONTINUE;
        }

        pDecoder->Feed(pChunkData, chunkSamples, feedType);
        decodedSamples = pDecoder->Decode(tempTracks, chunkSamples);
        totalDecodedSamples += decodedSamples;
        curSample += decodedSamples;
        pDataBegin += chunkBytes;
    }

    return totalDecodedSamples;
}

unsigned int XmaCopyBitsSkippingPacketHeaders(void* pSrc, unsigned int srcBitOffset, void* pDest, unsigned int dstBitOffset, unsigned int numBitsRequested, int forceLastBit, int* pLastBitWas, bool* bSkipCurrentPacket)
{
    BitGetter bitGetter;
    unsigned char* pSrcByte = (unsigned char*)pSrc + (srcBitOffset >> 3);
    unsigned char* pDestByte = (unsigned char*)pDest + (dstBitOffset >> 3);
    unsigned int ret = 8 * (srcBitOffset >> 3);
    *pLastBitWas = -1;
    bitGetter.SetBitBuffer(pSrcByte);
    bitGetter.GetBits(srcBitOffset % 8);
    unsigned int numPacketsSpanned = ((srcBitOffset + numBitsRequested) >> 14) - (srcBitOffset >> 14);
    unsigned int i;

    for (i = 0; i < numPacketsSpanned + 1; i++)
    {
        unsigned int numBits = (((srcBitOffset >> 14) + 1) << 14) - srcBitOffset;

        if (numBits > numBitsRequested)
        {
            numBits = numBitsRequested;
        }

        numBitsRequested -= numBits;
        srcBitOffset += numBits;

        if (dstBitOffset % 8)
        {
            unsigned int roundUpBits = 8 - dstBitOffset % 8;
            unsigned int copyBits = 0;
            int byteIncrement = 0;

            if (roundUpBits > numBits)
            {
                byteIncrement = 0;
                copyBits = numBits;
                roundUpBits -= numBits;
            }
            else
            {
                copyBits = roundUpBits;
                roundUpBits = 0;
                byteIncrement = 1;
            }

            *pDestByte &= ~((1 << (roundUpBits + copyBits)) - 1);
            unsigned char newBits = bitGetter.GetBits(copyBits);
            *pLastBitWas = newBits & 1;
            newBits <<= roundUpBits;
            *pDestByte |= newBits;
            pDestByte += byteIncrement;
            numBits -= copyBits;
            dstBitOffset += copyBits;
        }

        while (numBits >= 8)
        {
            *pDestByte = bitGetter.GetBits(8);
            numBits -= 8;
            dstBitOffset += 8;
            *pLastBitWas = *pDestByte++ & 1;
        }

        if (numBits)
        {
            unsigned int val = bitGetter.GetBits(numBits);
            *pLastBitWas = val & 1;
            *pDestByte = (val << (8 - numBits)) & 0xFF;
            dstBitOffset += numBits;
        }

        if (!((bitGetter.GetBitPosition() + ret) % 0x4000))
        {
            bitGetter.GetBits(32);
            srcBitOffset += 32;
            *bSkipCurrentPacket = 0;
        }
    }

    if (!forceLastBit)
    {
        if (dstBitOffset % 8)
        {
            unsigned int mask = ~((1 << (9 - dstBitOffset % 8)) - 1);
            *pDestByte &= (unsigned char)mask;
        }
        else
        {
            *--pDestByte &= (unsigned char)~1;
            pDestByte++;
        }
    }
    else if (forceLastBit == 1)
    {
        if (dstBitOffset % 8)
        {
            *pDestByte |= 1 << (8 - dstBitOffset % 8);
        }
        else
        {
            *--pDestByte |= 1;
            pDestByte++;
        }
    }

    ret += bitGetter.GetBitPosition();
    return ret;
}

int ExportWriteSndPlayerHelper(SINSTANCE* pSInstance, FileHandle** pGstream, SSOUND* pSSound, EncoderHelper* pEncoderhelper, int curFrame, int requestedNumFrames, int isstreamdata, int flush, bool isNewFeed, int writeChunkHeader, int* totalEncodedFrames, int* pStreamChanged, char* newfilename, int* pGigaHeaderNumFrames)
{
    const float STREAMCHUNKSIZE = 2048.0f;
    int totalBytes = 0;
    int startTrack = curFrame;
    int numframes = requestedNumFrames;
    __int64 gposLastHeader = 0;
    __int64 gposLastPair = 0;
    unsigned int blockBytesFlagged = 0;
    *totalEncodedFrames = 0;
    FileHandle* pFileHandle = *pGstream;

    if (requestedNumFrames <= 0)
    {
        return requestedNumFrames;
    }

    short** pSrcTracks = (short**)Allocator::Alloc(4 * pSSound->numchannels);

    if (pSSound->samplerep == 28 && isstreamdata)
    {
        unsigned int bytesWrittenThisChunk = 0;

        while (requestedNumFrames)
        {
            for (int k = 0; k < pSSound->numchannels; k++)
            {
                pSrcTracks[k] = &pSSound->track[k][startTrack];
            }

            unsigned char* pEncodedData = 0;
            unsigned char* pEncodedDataFlush = 0;
            int encodedBytes = 0;
            int flushBytes = 0;
            unsigned char* pSeekData = 0;
            unsigned char* pSeekDataFlush = 0;
            int seekDataBytes = 0;
            int seekDataFlushBytes = 0;
            unsigned int i = 0;
            int encodedSamples = pEncoderhelper->Encode(pSrcTracks, &pEncodedData, numframes, &encodedBytes, pSSound, &pSeekData, &seekDataBytes);
            flush = 1;

            if (encodedSamples <= 0 && flush == 0)
            {
                goto abortHelper;
            }

            requestedNumFrames -= numframes;
            startTrack += numframes;

            unsigned int blockBytes;

            if (requestedNumFrames <= 0)
            {
                if (flush)
                {
                    encodedSamples += pEncoderhelper->Flush(&pEncodedDataFlush, &flushBytes, &pSeekDataFlush, &seekDataFlushBytes);
                }

                blockBytesFlagged = encodedBytes + flushBytes + 8;
                blockBytes = blockBytesFlagged;
                blockBytesFlagged = blockBytesFlagged | 0x80000000;
                encodedBytes += flushBytes;
            }
            else
            {
                blockBytesFlagged = encodedBytes + 8;
                blockBytes = blockBytesFlagged;
            }

            unsigned int numStereoPairs = (pSSound->numchannels + 1) >> 1;
            unsigned int srcBitOffset[32] = { 0 };
            unsigned int dstBitOffset[32] = { 0 };
            unsigned int numBytesTotal[32] = { 0 };
            unsigned char* pSrcData[32] = { 0 };
            unsigned int decoderTableVal = 0;
            unsigned int numPacketsOutput = 0;
            unsigned int encodedDataIndex = 0;
            srcBitOffset[0] = 32;
            pSrcData[0] = pEncodedDataFlush + 4;

            for (i = 0; i < numStereoPairs; i++)
            {
                unsigned int val = 0;
                unsigned int temp = *(unsigned int*)&pEncodedDataFlush[encodedDataIndex];
                PutM(&val, temp, 4);
                numBytesTotal[i] = val >> 2;

                if (i < numStereoPairs - 1)
                {
                    encodedDataIndex += numBytesTotal[i];
                    srcBitOffset[i + 1] += 32;
                    pSrcData[i + 1] = &pEncodedDataFlush[encodedDataIndex + 4];
                }

                decoderTableVal = val & 3;
            }

            unsigned int numSamplesToWrite = numframes + 384;
            unsigned int samplesWritten = curFrame;
            bool bFirstChunk = true;
            bool bSkipCurrentPacket = true;
            unsigned int gigaHeaderNumFrames = pSSound->samplerate * pSSound->gigaInRamPeriod;
            gigaHeaderNumFrames += 384;
            gigaHeaderNumFrames += 511;
            gigaHeaderNumFrames = gigaHeaderNumFrames >> 9 << 9;
            gigaHeaderNumFrames -= 384;

            if (samplesWritten >= gigaHeaderNumFrames)
            {
            }
            else
            {
                gigaHeaderNumFrames = pSSound->samplerate * pSSound->gigaInRamPeriod;
                gigaHeaderNumFrames += 384;
                gigaHeaderNumFrames += 511;
                gigaHeaderNumFrames -= samplesWritten;
                gigaHeaderNumFrames = gigaHeaderNumFrames >> 9 << 9;
                gigaHeaderNumFrames += samplesWritten;
                gigaHeaderNumFrames -= 384;
            }

            *pGigaHeaderNumFrames = gigaHeaderNumFrames;

            while (numSamplesToWrite)
            {
                bytesWrittenThisChunk = 0;
                unsigned int samplesToWriteThisChunk = 5120;

                if ((pSSound->playloc & 0x2000) != 0 && samplesWritten < gigaHeaderNumFrames)
                {
                    isstreamdata = 0;
                    samplesToWriteThisChunk = (gigaHeaderNumFrames - samplesWritten) + (bFirstChunk ? 0x180 : 0);
                }

                if (samplesToWriteThisChunk > numSamplesToWrite)
                {
                    samplesToWriteThisChunk = numSamplesToWrite;
                }

                unsigned int framesToWriteThisChunk = (samplesToWriteThisChunk + 511) >> 9;

                if (writeChunkHeader && blockBytes > 8)
                {
                    gposLastHeader = FileIO::Tell(pFileHandle);
                    SndPlayerChunkHeader chunkHeader;
                    PutM(&chunkHeader, 0, 4);
                    PutM(&chunkHeader.samples, samplesToWriteThisChunk, 4);
                    FileIO::Write(pFileHandle, &chunkHeader, 8);
                }

                for (i = 0; i < numStereoPairs; i++)
                {
                    unsigned int byteswritenThisPair = 0;
                    unsigned int packetHeader = 0;
                    unsigned int frame = 0;
                    gposLastPair = FileIO::Tell(pFileHandle);
                    unsigned int tempXmaPairSize = 0;
                    FileIO::Write(pFileHandle, &tempXmaPairSize, 4);
                    bytesWrittenThisChunk += 4;
                    totalBytes += 4;
                    char packetBuffer[2048];
                    memset(packetBuffer, 0, sizeof(packetBuffer));
                    PutM(&packetHeader, 0x8000000, 4);
                    *(unsigned int*)packetBuffer = packetHeader;
                    dstBitOffset[i] = 32;

                    for (frame = 0; frame < framesToWriteThisChunk; frame++)
                    {
                        unsigned int frameSize = 0;
                        int lastCopiedBit = -1;
                        int srcByteOffset = srcBitOffset[i] >> 3;
                        int byte0 = srcByteOffset;
                        int byte1 = srcByteOffset + 1;
                        int byte2 = srcByteOffset + 2;

                        if ((byte1 & 0x7FF) == 0)
                        {
                            byte1 += 4;
                            byte2 += 4;
                        }
                        else if ((byte2 & 0x7FF) == 0)
                        {
                            byte2 += 4;
                        }

                        frameSize = (pSrcData[i][byte0] << 16) | (pSrcData[i][byte1] << 8) | pSrcData[i][byte2];
                        frameSize >>= (9 - srcBitOffset[i] % 8);
                        frameSize &= 0x7FFF;
                        int forceLastBit = frame != framesToWriteThisChunk - 1;

                        if (frameSize + dstBitOffset[i] <= 0x4000)
                        {
                            int myForceLastBit;

                            if (frameSize + dstBitOffset[i] == 0x4000)
                            {
                                myForceLastBit = 0;
                            }
                            else
                            {
                                myForceLastBit = forceLastBit;
                            }

                            srcBitOffset[i] = XmaCopyBitsSkippingPacketHeaders(pSrcData[i], srcBitOffset[i], packetBuffer, dstBitOffset[i], frameSize, myForceLastBit, &lastCopiedBit, &bSkipCurrentPacket);
                            dstBitOffset[i] += frameSize;

                            if (!lastCopiedBit)
                            {
                                if (!bSkipCurrentPacket)
                                {
                                    bSkipCurrentPacket = 1;
                                }
                                else
                                {
                                    if (srcBitOffset[i] % 0x4000)
                                    {
                                        srcBitOffset[i] &= 0xFFFFC000;
                                        srcBitOffset[i] += 0x4000;
                                    }

                                    srcBitOffset[i] += 32;
                                    bSkipCurrentPacket = 1;
                                }
                            }
                        }
                        else
                        {
                            unsigned int bitsLeft = 0x4000 - dstBitOffset[i];
                            srcBitOffset[i] = XmaCopyBitsSkippingPacketHeaders(pSrcData[i], srcBitOffset[i], packetBuffer, dstBitOffset[i], bitsLeft, -1, &lastCopiedBit, &bSkipCurrentPacket);
                            packetHeader = 0;
                            FileIO::Write(pFileHandle, packetBuffer, 2048);
                            byteswritenThisPair += 2048;
                            bytesWrittenThisChunk += 2048;
                            totalBytes += 2048;
                            memset(packetBuffer, 0, 2048);
                            unsigned int temp = 0;

                            if (forceLastBit)
                            {
                                temp = 0x8000000;

                                if (bitsLeft)
                                {
                                    temp |= (frameSize - bitsLeft) << 11;
                                }
                            }
                            else
                            {
                                temp = 0x22000000;
                            }

                            PutM(&packetHeader, temp, 4);
                            *(unsigned int*)packetBuffer = packetHeader;
                            dstBitOffset[i] = 32;
                            int myForceLastBit;

                            if (bitsLeft > 0)
                            {
                                myForceLastBit = 0;
                            }
                            else
                            {
                                myForceLastBit = forceLastBit;
                            }

                            srcBitOffset[i] = XmaCopyBitsSkippingPacketHeaders(pSrcData[i], srcBitOffset[i], packetBuffer, dstBitOffset[i], frameSize - bitsLeft, myForceLastBit, &lastCopiedBit, &bSkipCurrentPacket);

                            if (!lastCopiedBit)
                            {
                                if (!bSkipCurrentPacket)
                                {
                                    bSkipCurrentPacket = 1;
                                }
                                else
                                {
                                    if (srcBitOffset[i] % 0x4000)
                                    {
                                        srcBitOffset[i] &= 0xFFFFC000;
                                        srcBitOffset[i] += 0x4000;
                                    }

                                    srcBitOffset[i] += 32;
                                    bSkipCurrentPacket = 1;
                                }
                            }

                            dstBitOffset[i] += frameSize - bitsLeft;
                        }
                    }

                    FileIO::Write(pFileHandle, packetBuffer, (dstBitOffset[i] + 7) >> 3);
                    bytesWrittenThisChunk += (dstBitOffset[i] + 7) >> 3;
                    totalBytes += (dstBitOffset[i] + 7) >> 3;
                    byteswritenThisPair += (dstBitOffset[i] + 7) >> 3;
                    __int64 tempPos = FileIO::Tell(pFileHandle);
                    FileIO::Seek(pFileHandle, gposLastPair);
                    unsigned int sizeAndTable = byteswritenThisPair;
                    sizeAndTable += 4;
                    sizeAndTable <<= 2;
                    sizeAndTable |= decoderTableVal;
                    unsigned int temp2 = sizeAndTable;
                    PutM(&sizeAndTable, temp2, 4);
                    FileIO::Write(pFileHandle, &sizeAndTable, 4);
                    FileIO::Seek(pFileHandle, tempPos);
                }

                numSamplesToWrite -= samplesToWriteThisChunk;
                samplesWritten += samplesToWriteThisChunk;
                __int64 tempPos = FileIO::Tell(pFileHandle);
                FileIO::Seek(pFileHandle, gposLastHeader);
                bytesWrittenThisChunk += 8;
                totalBytes += 8;
                int padBytes = 0;
                int lastChunkFlag = 0;

                if (!numSamplesToWrite)
                {
                    padBytes = AlignUp(totalBytes, 64) - totalBytes;

                    if ((pSSound->playloc & 0x2000) == 0 || samplesWritten > gigaHeaderNumFrames + (bFirstChunk ? 0x180 : 0))
                    {
                        lastChunkFlag = 0x80000000;
                    }
                }

                unsigned int validSamplesInChunk = samplesToWriteThisChunk;

                if (bFirstChunk)
                {
                    validSamplesInChunk = samplesToWriteThisChunk - 384;
                }

                SndPlayerChunkHeader chunkHeader;
                PutM(&chunkHeader, lastChunkFlag | (bytesWrittenThisChunk + padBytes), 4);
                PutM(&chunkHeader.samples, validSamplesInChunk, 4);
                FileIO::Write(pFileHandle, &chunkHeader, 8);
                FileIO::Seek(pFileHandle, tempPos);

                if (padBytes > 0)
                {
                    char padding[64];
                    memset(padding, 0, sizeof(padding));
                    FileIO::Write(pFileHandle, padding, padBytes);
                }

                numPacketsOutput++;

                if ((pSSound->playloc & 0x2000) != 0 && samplesWritten == gigaHeaderNumFrames + (bFirstChunk ? 0x180 : 0) && numSamplesToWrite)
                {
                    char finalHeaderfilename[512];
                    char finalStreamfilename[512];
                    AddExtToSndPlayerFile(finalHeaderfilename, pSInstance->fileName, 0, "snr");
                    AddExtToSndPlayerFile(finalStreamfilename, pSInstance->fileName, 0, "sns");
                    AddExtToSndPlayerFile(newfilename, pSInstance->fileName, "H", "snr");
                    FileIO::Close(pFileHandle);
                    rename(newfilename, finalHeaderfilename);
                    AddExtToSndPlayerFile(newfilename, pSInstance->fileName, "S", "sns");
                    remove(newfilename);
                    *pGstream = FileIO::WOpen(newfilename);
                    pFileHandle = *pGstream;
                    isstreamdata = 1;
                    *pStreamChanged = 1;
                    totalBytes = 0;
                }

                bFirstChunk = false;
            }

            *totalEncodedFrames += encodedSamples;
        }
    }
    else
    {
        while (requestedNumFrames)
        {
            if (isstreamdata)
            {
                numframes = (STREAMCHUNKSIZE - 8.0) / pEncoderhelper->GetAverageDataRate() * pSSound->samplerate;

                if (numframes > requestedNumFrames)
                {
                    numframes = requestedNumFrames;
                }
            }

            for (int k = 0; k < pSSound->numchannels; k++)
            {
                pSrcTracks[k] = &pSSound->track[k][startTrack];
            }

            unsigned char* pEncodedData = 0;
            unsigned char* pEncodedDataFlush = 0;
            int encodedBytes = 0;
            int flushBytes = 0;
            unsigned char* pSeekData = 0;
            unsigned char* pSeekDataFlush = 0;
            int seekDataBytes = 0;
            int seekDataFlushBytes = 0;
            int encodedSamples = pEncoderhelper->Encode(pSrcTracks, &pEncodedData, numframes, &encodedBytes, pSSound, &pSeekData, &seekDataBytes);

            if (pSSound->samplerep == 28)
            {
                flush = 1;
            }

            if (encodedSamples <= 0 && !flush)
            {
                goto abortHelper;
            }

            requestedNumFrames -= numframes;
            startTrack += numframes;
            int padBytes = 0;
            unsigned int blockBytes;

            if (encodedBytes > 0 || flush)
            {
                if (requestedNumFrames <= 0)
                {
                    if (flush)
                    {
                        encodedSamples += pEncoderhelper->Flush(&pEncodedDataFlush, &flushBytes, &pSeekDataFlush, &seekDataFlushBytes);
                    }

                    blockBytesFlagged = encodedBytes + flushBytes + 8;
                    blockBytes = blockBytesFlagged;

                    if (isstreamdata)
                    {
                        blockBytesFlagged |= 0x80000000;
                    }
                }
                else
                {
                    blockBytesFlagged = encodedBytes + 8;
                    blockBytes = blockBytesFlagged;
                }

                totalBytes += encodedBytes + flushBytes;
                bool wasDataProduced = blockBytes > 8;

                if (writeChunkHeader && wasDataProduced)
                {
                    totalBytes += 8;

                    if (requestedNumFrames <= 0 && isstreamdata)
                    {
                        padBytes = 64 - totalBytes % 64;
                        blockBytes += padBytes;
                        blockBytesFlagged += padBytes;
                        totalBytes += padBytes;
                    }

                    gposLastHeader = FileIO::Tell(pFileHandle);
                    SndPlayerChunkHeader chunkHeader;
                    PutM(&chunkHeader, blockBytesFlagged, 4);
                    PutM(&chunkHeader.samples, encodedSamples, 4);
                    FileIO::Write(pFileHandle, &chunkHeader, 8);
                }
            }

            *totalEncodedFrames += encodedSamples;
            FileIO::Write(pFileHandle, pEncodedData, encodedBytes);

            if (requestedNumFrames <= 0 && flush)
            {
                FileIO::Write(pFileHandle, pEncodedDataFlush, flushBytes);
            }

            if (padBytes)
            {
                char padding[64];
                memset(padding, 0, 64);
                FileIO::Write(pFileHandle, padding, padBytes);
            }
        }
    }

abortHelper:
    Allocator::Free(pSrcTracks);
    return totalBytes;
}

int AboutSndPlayer(SABOUT* pSAbout)
{
    pSAbout->imp.commonvers[0] = 0;
    pSAbout->imp.commonvers[1] = -1;
    pSAbout->imp.platformvers[0] = -1;
    pSAbout->imp.samplereps[0] = 29;
    pSAbout->imp.samplereps[1] = 30;
    pSAbout->imp.samplereps[2] = 1;
    pSAbout->imp.samplereps[3] = 31;
    pSAbout->imp.samplereps[4] = 32;
    pSAbout->imp.samplereps[5] = -1;
    pSAbout->exp.commonvers[0] = 0;
    pSAbout->exp.commonvers[1] = -1;
    pSAbout->exp.platformvers[0] = -1;
    pSAbout->exp.samplereps[0] = 29;
    pSAbout->exp.samplereps[1] = 1;
    pSAbout->exp.samplereps[2] = 30;
    pSAbout->exp.samplereps[3] = 31;
    pSAbout->exp.samplereps[4] = 32;
    pSAbout->exp.samplereps[5] = -1;

    pSAbout->maxelements = 1;
    pSAbout->maxtimbres = 1;
    pSAbout->maxchannels = 64;
    pSAbout->canimport = 1;
    pSAbout->canexport = 1;
    pSAbout->vbr = 1;
    pSAbout->loop = 1;
    pSAbout->playlocram = 1;
    pSAbout->playlocstream = 1;
    pSAbout->playlocgigasample = 1;
    strcpy(pSAbout->formatword, "sndplayer");
    strcpy(pSAbout->formatname, "SND Player");
    return 1;
}

char srcSndPlayerSnsName[512];

int ImportIsSndPlayer(const char* pFileName, __int64 fileOffset, FileHandle* pgs)
{
    int retVal = 50;
    FileHandle* streamFile = 0;

    if (!pgs)
    {
        return 0;
    }

    if (FileIO::Length(pgs) < 8)
    {
        return 0;
    }

    char tempSIMEXfilename[512];
    strncpy(tempSIMEXfilename, pFileName, 512);
    _strrev(tempSIMEXfilename);

    if (tempSIMEXfilename[0] == 'r' && tempSIMEXfilename[1] == 'n' && tempSIMEXfilename[2] == 's' && tempSIMEXfilename[3] == '.')
    {
        retVal = 90;
    }

    char packedHeader[64];
    FileIO::Read(pgs, packedHeader, 64);
    SndPlayerHeaderInfo headerInfo;

    if (ReadSndPlayerFileHeader(&headerInfo, packedHeader) < 0)
    {
        return 0;
    }

    if (headerInfo.playType == 1 || headerInfo.playType == 2)
    {
        memset(srcSndPlayerSnsName, 0, sizeof(srcSndPlayerSnsName));
        ChangeSndPlayerFileExtension(srcSndPlayerSnsName, pFileName, "sns");
    }

    if (headerInfo.playType == 1)
    {
        streamFile = FileIO::Open(srcSndPlayerSnsName);

        if (!streamFile)
        {
            retVal = 0;
        }

        FileIO::Close(streamFile);
    }

    if (headerInfo.playType == 2)
    {
        streamFile = FileIO::Open(srcSndPlayerSnsName);

        if (!streamFile && headerInfo.gigaSamplesInRam < headerInfo.numSamples)
        {
            retVal = 0;
        }

        FileIO::Close(streamFile);
    }

    return retVal;
}

int ImportInfoSndPlayer(SINSTANCE* psi, SINFO** ppsinfo, int element)
{
    *ppsinfo = 0;

    if (element)
    {
        SIMEXI_setlasterr("An attempt to get info from a patch element other than 0 was made.");
        return 0;
    }

    *ppsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));

    if (!*ppsinfo)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for an SINFO structure.");
        return 0;
    }

    SIMEX_defaultsinfo(*ppsinfo);
    SSOUND* pSSound = (*ppsinfo)->sound[0];
    FileIO::Seek(psi->pFileHandle, 0);

    if (FileIO::Length(psi->pFileHandle) < 8)
    {
        SIMEXI_setlasterr("Invalid SNR file detected.");
        return 0;
    }

    char packedHeader[64];
    FileIO::Read(psi->pFileHandle, packedHeader, 64);

    SndPlayerHeaderInfo headerInfo;

    if (ReadSndPlayerFileHeader(&headerInfo, packedHeader) < 0)
    {
        SIMEXI_setlasterr("Couldn't parse the SndPlayer header.");
        return 0;
    }

    pSSound->platformver = headerInfo.version;
    pSSound->gigaInRamPeriod = (double)headerInfo.gigaSamplesInRam / (double)headerInfo.sampleRate;
    pSSound->length = headerInfo.numSamples;
    pSSound->numchannels = headerInfo.numChannels;
    pSSound->samplerate = headerInfo.sampleRate;

    if (headerInfo.loopStart >= 0)
    {
        pSSound->sustainstart = headerInfo.loopStart;
        pSSound->sustainend = headerInfo.numSamples - 1;
    }

    if (!headerInfo.playType)
    {
        pSSound->playloc = 0x800;
    }
    else if (headerInfo.playType == 1)
    {
        pSSound->playloc = 0x1000;
    }
    else
    {
        pSSound->playloc = 0x2000;
    }

    if (headerInfo.codec == 4)
    {
        pSSound->samplerep = 29;
    }
    else if (headerInfo.codec == 0)
    {
        pSSound->samplerep = 24;
    }
    else if (headerInfo.codec == 6)
    {
        pSSound->samplerep = 32;
    }
    else if (headerInfo.codec == 7)
    {
        pSSound->samplerep = 32;
    }
    else if (headerInfo.codec == 5)
    {
        pSSound->samplerep = 30;
    }
    else if (headerInfo.codec == 1)
    {
        pSSound->samplerep = 25;
    }
    else if (headerInfo.codec == 3)
    {
        pSSound->samplerep = 28;
    }
    else
    {
        pSSound->samplerep = 1;
    }

    return 1;
}

int ImportReadSndPlayer(SINSTANCE* psi, SINFO* psinfo, int element)
{
    SCOMPSTATE* pcompstate[64] = { 0 };
    __int64 sampleoffsets[64] = { 0 };
    char* snrContent = 0;
    DecoderExtended* pDecoder = 0;
    int ret = 0;
    element = element;
    SSOUND* pSSound = psinfo->sound[0];
    int dataSize;
    Guid guid;
    System* pSndSystem;
    DecoderRegistry* pDecoderRegistry;
    char* pDataPointer;
    void* decoderHandle;
    int samplesDecoded = 0;

    if (SIMEXI_allocatecompstate(pcompstate, 64) < 0)
    {
        goto abort;
    }

    if (SIMEXI_allocatetracks(pSSound) < 0)
    {
        goto abort;
    }

    FileIO::Seek(psi->pFileHandle, 0);
    dataSize = FileIO::Length(psi->pFileHandle);

    if (dataSize < 8)
    {
        SIMEXI_setlasterr("Invalid SNR file detected.");
        goto abort;
    }

    snrContent = (char*)Allocator::Alloc(dataSize);

    if (!snrContent)
    {
        SIMEXI_setlasterr("Couldn't allocate memory.");
        goto abort;
    }

    FileIO::Read(psi->pFileHandle, snrContent, dataSize);
    SndPlayerHeaderInfo headerInfo;

    if (ReadSndPlayerFileHeader(&headerInfo, snrContent) < 0)
    {
        SIMEXI_setlasterr("Couldn't parse the SndPlayer header.");
        goto abort;
    }

    guid = 0;

    if (pSSound->samplerep == 29)
    {
        guid = 'Xas1';
    }
    else if (pSSound->samplerep == 1)
    {
        guid = 'P6B0';
    }
    else if (pSSound->samplerep == 31)
    {
        guid = 'L32P';
    }
    else if (pSSound->samplerep == 32)
    {
        guid = 'L32S';
    }
    else if (pSSound->samplerep == 30)
    {
        guid = 'EL31';
    }

    pSndSystem = System::GetInstance();
    pSndSystem->Lock();
    pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    decoderHandle = pDecoderRegistry->GetDecoderHandle(guid);
    pDecoder = pDecoderRegistry->DecoderExtendedFactory(decoderHandle, headerInfo.numChannels, 1, pSndSystem);
    pSndSystem->Unlock();
    pDataPointer = headerInfo.pSampleData;
    int sampleDataSize;

    if (headerInfo.playType == 0 || headerInfo.playType == 2)
    {
        sampleoffsets[0] = headerInfo.pSampleData - snrContent;
        sampleDataSize = dataSize - (int)sampleoffsets[0];
    }
    else
    {
        sampleoffsets[0] = headerInfo.pSampleData - snrContent;
        sampleDataSize = 0;
    }

    short* tracks[64];

    for (int count = 0; count < 2; count++)
    {
        for (int i = 0; i < headerInfo.numChannels; i++)
        {
            tracks[i] = &pSSound->track[i][samplesDecoded];
        }

        samplesDecoded += ImportReadSndPlayerHelper(headerInfo, pDecoder, samplesDecoded, pDataPointer, sampleDataSize, tracks);

        if (samplesDecoded == headerInfo.numSamples)
        {
            ret = 1;
            count = 2;
        }
        else
        {
            FileHandle* streamFile;

            if (samplesDecoded < headerInfo.numSamples && !count)
            {
                Allocator::Free(snrContent);
                streamFile = FileIO::Open(srcSndPlayerSnsName);

                if (!streamFile)
                {
                    SIMEXI_setlasterr("Couldn't open SndPlayer stream file.");
                    goto abort;
                }

                sampleDataSize = FileIO::Length(streamFile);

                if (sampleDataSize <= 0)
                {
                    SIMEXI_setlasterr("Invalid SndPlayer stream file detected.");
                    FileIO::Close(streamFile);
                    goto abort;
                }

                snrContent = (char*)Allocator::Alloc(sampleDataSize);

                if (!snrContent)
                {
                    SIMEXI_setlasterr("Couldn't allocate memory.");
                    FileIO::Close(streamFile);
                    goto abort;
                }

                FileIO::Read(streamFile, snrContent, sampleDataSize);
                FileIO::Close(streamFile);
                pDataPointer = snrContent;
                ret = 1;
            }
            else
            {
                SIMEXI_setlasterr("Couldn't import all data from SndPlayer files.");
                goto abort;
            }
        }
    }

abort:
    if (snrContent)
    {
        Allocator::Free(snrContent);
    }

    SIMEXI_freecompstate(pcompstate, 64);

    if (pDecoder)
    {
        pDecoder->Release();
    }

    return ret;
}

int ExportOpenSndPlayer(SINSTANCE* pSInstance)
{
    return 1;
}

static int WriteHeaderSndPlayer(SSOUND* pSSound, FileHandle* pGStream, int offSetLoopStart, int gigaHeaderNumFrames)
{
    BitPut bitPut;
    bitPut.PutBits(0, 4);

    if (pSSound->samplerep == 29)
    {
        bitPut.PutBits(4, 4);
    }
    else if (pSSound->samplerep == 24)
    {
        SIMEXI_setlasterr("Error writing SndPlayer header for XAS samplerep.");
        return -1;
    }
    else if (pSSound->samplerep == 31)
    {
        bitPut.PutBits(6, 4);
    }
    else if (pSSound->samplerep == 32)
    {
        bitPut.PutBits(7, 4);
    }
    else if (pSSound->samplerep == 30)
    {
        bitPut.PutBits(5, 4);
    }
    else if (pSSound->samplerep == 25)
    {
        SIMEXI_setlasterr("Error writing SndPlayer header for SND_SR_EALAYER3_INT samplerep.");
        return -1;
    }
    else if (pSSound->samplerep == 28)
    {
        bitPut.PutBits(3, 4);
    }
    else
    {
        bitPut.PutBits(2, 4);
    }

    bitPut.PutBits(pSSound->numchannels - 1, 6);
    bitPut.PutBits(pSSound->samplerate, 18);

    if ((pSSound->playloc & 0x800) != 0 || !pSSound->playloc)
    {
        bitPut.PutBits(0, 2);
    }
    else if ((pSSound->playloc & 0x1000) != 0)
    {
        bitPut.PutBits(1, 2);
    }
    else
    {
        bitPut.PutBits(2, 2);
    }

    bitPut.PutBits(pSSound->sustainstart >= 0, 1);

    if (pSSound->sustainstart >= 0)
    {
        bitPut.PutBits(pSSound->sustainend + 1, 29);
    }
    else
    {
        bitPut.PutBits(pSSound->length, 29);
    }

    if (pSSound->sustainstart >= 0)
    {
        bitPut.PutBits(pSSound->sustainstart, 32);
    }

    if ((pSSound->playloc & 0x1000) != 0 && pSSound->sustainstart >= 0)
    {
        unsigned int bitsWritten = bitPut.GetBitPosition();
        bitPut.PutBits(offSetLoopStart, 32);
    }

    if ((pSSound->playloc & 0x2000) != 0)
    {
        unsigned int bitsWritten = bitPut.GetBitPosition();
        bitPut.PutBits(gigaHeaderNumFrames, 32);

        if (pSSound->sustainstart >= gigaHeaderNumFrames)
        {
            bitsWritten = bitPut.GetBitPosition();

            if (pSSound->sustainstart == gigaHeaderNumFrames)
            {
                bitPut.PutBits(0, 32);
            }
            else
            {
                bitPut.PutBits(offSetLoopStart, 32);
            }
        }
    }

    char headerBuf[64];
    int headerBytes = bitPut.GetBytes(headerBuf, 64);

    if (!FileIO::Write(pGStream, headerBuf, headerBytes))
    {
        SIMEXI_setlasterr("Error writing SndPlayer header.");
        return -1;
    }

    return 0;
}

int ExportWriteSndPlayer(SINSTANCE* pSInstance, SINFO* pSinfo, int element)
{
    SCOMPSTATE* pcompstate[64] = { 0 };
    int ret = 0;
    int offSetLoopStart = -1;
    int gigaHeaderNumFrames = 0;
    int curFrame = 0;
    int actualGigaSamples = 0;
    int sustainstartdata = 0;
    int flush = 0;
    int streamChanged = 0;
    int gigaHeadEndReached = 0;
    bool isNewFeed = true;
    int numframes;
    int totalframes;
    SSOUND* pSSound;
    EncoderHelper* pEncoderhelper = 0;
    FileHandle* pFileHandle;
    unsigned int filelength;
    int killdest = 0;
    unsigned char* pFileData = 0;

    if (element)
    {
        SIMEXI_setlasterr("TBD files can only contain element 0.");
        goto abort;
    }

    pSSound = pSinfo->sound[0];

    if (SIMEXI_allocatecompstate(pcompstate, 64) < 0)
    {
        goto abort;
    }

    if (pSSound->samplerep == 29)
    {
        pEncoderhelper = EncoderHelper::CreateInstance(pSSound, 'Xas1');
    }
    else if (pSSound->samplerep == 1)
    {
        pEncoderhelper = EncoderHelper::CreateInstance(pSSound, 'P6B0');
    }
    else if (pSSound->samplerep == 31)
    {
        pEncoderhelper = EncoderHelper::CreateInstance(pSSound, 'L32P');
    }
    else if (pSSound->samplerep == 32)
    {
        pEncoderhelper = EncoderHelper::CreateInstance(pSSound, 'L32S');
    }
    else if (pSSound->samplerep == 30)
    {
        pEncoderhelper = EncoderHelper::CreateInstance(pSSound, 'EL31');
    }

    char finalHeaderfilename[512];
    AddExtToSndPlayerFile(finalHeaderfilename, pSInstance->fileName, 0, "snr");
    remove(finalHeaderfilename);
    char finalStreamfilename[512];
    AddExtToSndPlayerFile(finalStreamfilename, pSInstance->fileName, 0, "sns");
    remove(finalStreamfilename);
    char newfilename[512];
    AddExtToSndPlayerFile(newfilename, pSInstance->fileName, "H", "snr");
    remove(newfilename);
    pFileHandle = FileIO::WOpen(newfilename);

    if (pSSound->sustainstart >= 0)
    {
        totalframes = pSSound->sustainend + 1;
    }
    else
    {
        totalframes = pSSound->length;
    }

    if ((pSSound->playloc & 0x800) != 0 || !pSSound->playloc || (pSSound->playloc & 0x1000) != 0)
    {
        if (pSSound->gigaInRamPeriod > 0.0)
        {
            SIMEXI_warningcb("    NOTE: This is not a Gigasample file, therefore the period of %f is ignored.\n", pSSound->gigaInRamPeriod);
        }
    }
    else
    {
        if (pSSound->gigaInRamPeriod <= 0.0)
        {
            SIMEXI_warningcb("    NOTE: No Gigasample in-RAM period specified. Using default of 1 second.\n");
            pSSound->gigaInRamPeriod = 1.0;
        }

        gigaHeaderNumFrames = pSSound->samplerate * pSSound->gigaInRamPeriod;

        if (gigaHeaderNumFrames > totalframes)
        {
            gigaHeaderNumFrames = totalframes;
        }
    }

    int isstreamdata;

    while (curFrame < totalframes)
    {
        if (!streamChanged && (pSSound->playloc == 0x1000 || gigaHeadEndReached && (pSSound->playloc & 0x2000) != 0))
        {
            FileIO::Close(pFileHandle);
            rename(newfilename, finalHeaderfilename);
            AddExtToSndPlayerFile(newfilename, pSInstance->fileName, "S", "sns");
            remove(newfilename);
            pFileHandle = FileIO::WOpen(newfilename);
            streamChanged = 1;
        }

        if ((pSSound->playloc & 0x800) != 0 || !pSSound->playloc || (pSSound->playloc & 0x1000) != 0)
        {
            isstreamdata = pSSound->playloc == 0x1000;

            if (pSSound->sustainstart >= 0)
            {
                if (curFrame < pSSound->sustainstart)
                {
                    numframes = pSSound->sustainstart;
                    sustainstartdata = 1;
                }
                else
                {
                    numframes = pSSound->sustainend - pSSound->sustainstart + 1;
                }
            }
            else
            {
                numframes = pSSound->length;
            }

            flush = 1;
        }
        else if (pSSound->samplerep == 28)
        {
            flush = 1;
            isstreamdata = 1;

            if (pSSound->sustainstart >= 0)
            {
                if (curFrame < pSSound->sustainstart)
                {
                    numframes = pSSound->sustainstart;

                    if (pSSound->sustainstart > gigaHeaderNumFrames)
                    {
                        sustainstartdata = 1;
                    }
                }
                else
                {
                    numframes = pSSound->sustainend - pSSound->sustainstart + 1;
                }
            }
            else
            {
                numframes = totalframes;
            }
        }
        else if (pSSound->sustainstart < 0)
        {
            if (curFrame < gigaHeaderNumFrames)
            {
                numframes = totalframes < gigaHeaderNumFrames ? totalframes : gigaHeaderNumFrames;
                isstreamdata = 0;
                flush = totalframes <= gigaHeaderNumFrames;
            }
            else
            {
                numframes = totalframes - curFrame;
                isstreamdata = 1;
                flush = 1;
            }
        }
        else if (curFrame < gigaHeaderNumFrames)
        {
            if (pSSound->sustainstart < gigaHeaderNumFrames)
            {
                numframes = curFrame < pSSound->sustainstart ? pSSound->sustainstart : gigaHeaderNumFrames - pSSound->sustainstart;
                sustainstartdata = curFrame < pSSound->sustainstart;
                isstreamdata = 0;
                flush = curFrame < pSSound->sustainstart;

                if (curFrame + numframes - 1 == pSSound->sustainend)
                {
                    flush = 1;
                }
            }
            else
            {
                numframes = gigaHeaderNumFrames;
                sustainstartdata = 1;
                isstreamdata = 0;
                flush = 0;
            }
        }
        else
        {
            if (curFrame < pSSound->sustainstart)
            {
                numframes = pSSound->sustainstart - curFrame;
                sustainstartdata = 1;
                isstreamdata = 1;
                flush = 1;
            }
            else
            {
                numframes = pSSound->sustainend - curFrame + 1;
                sustainstartdata = 0;
                isstreamdata = 1;
                flush = 1;
            }
        }

        __int64 gposBeforeExport = FileIO::Tell(pFileHandle);
        int encodedframes;
        int exportedChunkBytes = ExportWriteSndPlayerHelper(pSInstance, &pFileHandle, pSSound, pEncoderhelper, curFrame, numframes, isstreamdata, flush, isNewFeed, 1, &encodedframes, &streamChanged, newfilename, &gigaHeaderNumFrames);
        ret += exportedChunkBytes;
        isNewFeed = flush != 0;
        __int64 gposAfterExport;

        if (offSetLoopStart < 0)
        {
            if (sustainstartdata && isstreamdata)
            {
                gposAfterExport = FileIO::Tell(pFileHandle);
                offSetLoopStart = gposAfterExport - gposBeforeExport;
            }

            if (!pSSound->sustainstart)
            {
                offSetLoopStart = 0;
            }
        }

        curFrame += numframes;
        actualGigaSamples += encodedframes;
        float percent = curFrame * 100.0 / totalframes;
        SIMEXI_progresscb(FToI::Fast(percent));

        if (encodedframes < numframes)
        {
            if ((pSSound->playloc & 0x2000) != 0)
            {
                int writeHeader = encodedframes == 0;
                int tmpFrames = 0;

                while (tmpFrames <= 0 && curFrame < totalframes)
                {
                    if (++curFrame == pSSound->sustainstart)
                    {
                        flush = 1;
                    }

                    exportedChunkBytes += ExportWriteSndPlayerHelper(pSInstance, &pFileHandle, pSSound, pEncoderhelper, curFrame, 1, isstreamdata, flush, isNewFeed, writeHeader, &tmpFrames, &streamChanged, newfilename, &gigaHeaderNumFrames);
                    ret += exportedChunkBytes;
                    isNewFeed = flush != 0;
                }

                actualGigaSamples += tmpFrames;
                gigaHeaderNumFrames = actualGigaSamples;

                if (!writeHeader)
                {
                    gposAfterExport = FileIO::Tell(pFileHandle);
                    FileIO::Seek(pFileHandle, gposBeforeExport);
                    unsigned int tempChunkBytes;
                    PutM(&tempChunkBytes, exportedChunkBytes, 4);
                    FileIO::Write(pFileHandle, &tempChunkBytes, 4);
                    unsigned int tempChunkFrames;
                    PutM(&tempChunkFrames, encodedframes + tmpFrames, 4);
                    FileIO::Write(pFileHandle, &tempChunkFrames, 4);
                    FileIO::Seek(pFileHandle, gposAfterExport);
                }
            }
            else
            {
                SIMEXI_setlasterr("Unexpected behavior: Encoded less frames than requested in non-gigasample data export.");
                goto abort;
            }
        }

        if ((pSSound->playloc & 0x2000) != 0 && curFrame >= gigaHeaderNumFrames)
        {
            gigaHeadEndReached = 1;
        }
    }

    FileIO::Seek(pFileHandle, 0);

    if (!FileIO::Length(pFileHandle))
    {
        killdest = 1;
    }

    FileIO::Close(pFileHandle);

    if (killdest)
    {
        remove(newfilename);
    }
    else if (streamChanged)
    {
        rename(newfilename, finalStreamfilename);
    }
    else
    {
        rename(newfilename, finalHeaderfilename);
    }

    pEncoderhelper->Release();
    pFileHandle = FileIO::Open(finalHeaderfilename);

    if (!pFileHandle)
    {
        SIMEXI_setlasterr("Error reopening SndPlayer SNR file for header update.");
        ret = 0;
        goto abort;
    }

    filelength = FileIO::Length(pFileHandle);

    if (filelength > 0)
    {
        pFileData = (unsigned char*)Allocator::Alloc(filelength);
        FileIO::Read(pFileHandle, (char*)pFileData, filelength);
    }

    FileIO::Close(pFileHandle);
    remove(finalHeaderfilename);
    AddExtToSndPlayerFile(newfilename, pSInstance->fileName, "H", "snr");
    remove(newfilename);
    pFileHandle = FileIO::WOpen(newfilename);

    if (WriteHeaderSndPlayer(pSSound, pFileHandle, offSetLoopStart, gigaHeaderNumFrames) < 0)
    {
        SIMEXI_setlasterr("Error writing the header file.");
        ret = 0;
    }
    else
    {
        if (filelength > 0)
        {
            FileIO::Write(pFileHandle, pFileData, filelength);
            Allocator::Free(pFileData);
        }

        FileIO::Close(pFileHandle);
        rename(newfilename, finalHeaderfilename);
    }

abort:
    SIMEXI_freecompstate(pcompstate, 64);
    return ret;
}
