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

struct CHUNKHDR
{
    char id[4];
    char size[4];
};

struct RIFFHDR
{
    struct CHUNKHDR hdr;
    char fcctype[4];
};

struct WAVEHDR_SIMEX
{
    struct CHUNKHDR hdr;
    char wFormatTag[2];
    char nChannels[2];
    char nSamplesPerSec[4];
    char nAvgBytesPerSec[4];
    char nBlockAlign[2];
};

struct PCMWAVEHDR
{
    struct WAVEHDR_SIMEX wf;
    char wBitsPerSample[2];
};

struct PCMWAVEEXTENSIBLEHDR
{
    struct WAVEHDR_SIMEX wf;
    char wBitsPerSample[2];
    char wcbSize[2];
    char wValidBitsPerSample[2];
    char dwChannelMask[4];
    unsigned char SubFormat[16];
};

struct SAMPLERHDR
{
    struct CHUNKHDR hdr;
    char dwManufacturer[4];
    char dwProduct[4];
    char dwSamplePeriod[4];
    char dwMIDIUnityNote[4];
    char dwMIDIPitchFraction[4];
    char dwSMPTEFormat[4];
    char dwSMPTEOffset[4];
    char cSampleLoops[4];
    char cbSamplerData[4];
};

struct SAMPLELOOP
{
    char dwIdentifier[4];
    char dwType[4];
    char dwStart[4];
    char dwEnd[4];
    char dwFraction[4];
    char dwPlayCount[4];
};

void SIMEXI_remapWaveToITU(SSOUND* psound)
{
    short *ptrack;

    if (psound->numchannels == 4)
    {
        psound->track[0] = psound->track[0];
        psound->track[1] = psound->track[1];
        ptrack = psound->track[2];
        psound->track[2] = psound->track[3];
        psound->track[3] = ptrack;
    }

    if (psound->numchannels == 6)
    {
        psound->track[0] = psound->track[0];
        ptrack = psound->track[1];
        psound->track[1] = psound->track[2];
        psound->track[2] = ptrack;
        ptrack = psound->track[3];
        psound->track[3] = psound->track[5];
        psound->track[5] = psound->track[4];
        psound->track[4] = ptrack;
    }
}

void SIMEXI_remapITUToWave(SSOUND* psound)
{
    short *ptrack;

    if (psound->numchannels == 4)
    {
        psound->track[0] = psound->track[0];
        psound->track[1] = psound->track[1];
        ptrack = psound->track[3];
        psound->track[3] = psound->track[2];
        psound->track[2] = ptrack;
    }

    if (psound->numchannels == 6)
    {
        psound->track[0] = psound->track[0];
        ptrack = psound->track[2];
        psound->track[2] = psound->track[1];
        psound->track[1] = ptrack;
        ptrack = psound->track[5];
        psound->track[5] = psound->track[4];
        psound->track[4] = psound->track[3];
        psound->track[3] = ptrack;
    }
}

__int64 findchunk(FileHandle* pFileHandle, const char* chunkid, int* pNumDataChunksFound)
{
    int actualBytesRead;
    __int64 totalLength;
    __int64 fileOffset;
    __int64 orgPos;
    CHUNKHDR chunkHdr;

    totalLength = FileIO::Length(pFileHandle);
    orgPos = FileIO::Tell(pFileHandle);
    fileOffset = 0;
    *pNumDataChunksFound = 0;
    FileIO::Seek(pFileHandle, 0);

    while (fileOffset < totalLength)
    {
        actualBytesRead = FileIO::Read(pFileHandle, &chunkHdr, 8);

        if (actualBytesRead < 8u)
        {
            goto abortFindChunk;
        }

        if (strncmp(chunkHdr.id, chunkid, 4))
        {
            if (!strncmp(chunkHdr.id, "RIFF", 4))
            {
                fileOffset += 12;
            }
            else
            {
                fileOffset += GetI(chunkHdr.size, 4) + 8;
            }

            FileIO::Seek(pFileHandle, fileOffset);
        }
        else
        {
            if (!strncmp(chunkHdr.id, "data", 4) && GetI(chunkHdr.size, 4) <= 0)
            {
                SIMEXI_warningcb("Data Chunk with invalid size detected.");
                fileOffset += 8;
                (*pNumDataChunksFound)++;
                FileIO::Seek(pFileHandle, fileOffset);
            }
            else
            {
                FileIO::Seek(pFileHandle, fileOffset);
                return fileOffset;
            }
        }
    }

abortFindChunk:
    FileIO::Seek(pFileHandle, orgPos);
    return -1;
}

int aboutwave(SABOUT* pSAbout)
{
    pSAbout->imp.commonvers[0] = -1;
    pSAbout->imp.platformvers[0] = -1;
    pSAbout->imp.samplereps[0] = 19;
    pSAbout->imp.samplereps[1] = 0;
    pSAbout->imp.samplereps[2] = 11;
    pSAbout->imp.samplereps[3] = -1;
    pSAbout->exp.commonvers[0] = -1;
    pSAbout->exp.platformvers[0] = -1;
    pSAbout->exp.samplereps[0] = 0;
    pSAbout->exp.samplereps[1] = 11;
    pSAbout->exp.samplereps[2] = -1;
    pSAbout->maxelements = 1;
    pSAbout->maxtimbres = 1;
    pSAbout->maxchannels = 64;
    pSAbout->canimport = 1;
    pSAbout->canexport = 1;
    strcpy(pSAbout->formatword, "wave");
    strcpy(pSAbout->formatname, "Microsoft Wave");
    return 1;
}

int iswave(const char* pFileName, __int64 fileOffset, FileHandle* pFileHandle)
{
    CHUNKHDR chunkhdr;
    RIFFHDR riffhdr;
    PCMWAVEHDR pwh;

    if (!pFileHandle)
    {
        return 0;
    }

    if (FileIO::Read(pFileHandle, &riffhdr, 12) && !strncmp(riffhdr.hdr.id, "RIFF", 4) && !strncmp(riffhdr.fcctype, "WAVE", 4))
    {
        while (FileIO::Read(pFileHandle, &chunkhdr, 8))
        {
            if (strncmp(chunkhdr.id, "fmt ", 4))
            {
                FileIO::Seek(pFileHandle, FileIO::Tell(pFileHandle) + GetI(chunkhdr.size, 4));
            }
            else
            {
                FileIO::Seek(pFileHandle, FileIO::Tell(pFileHandle) - 8);
                break;
            }
        }

        if (FileIO::Read(pFileHandle, &pwh, 24) && (GetI(pwh.wf.wFormatTag, 2) == 1 || GetI(pwh.wf.wFormatTag, 2) == 65534))
        {
            return 100;
        }
    }

    return 0;
}

int infowave(SINSTANCE* psi, SINFO** ppsinfo, int element)
{
    RIFFHDR riffhdr;
    SAMPLERHDR smplhdr;
    SSOUND* psound;
    CHUNKHDR chunkhdr;
    SINFO* psinfo;
    PCMWAVEEXTENSIBLEHDR pcmwavehdr;
    __int64 offset;
    int numDataChunks;
    SAMPLELOOP smplloop;
    __int64 temp_pFileHandle_curpos;

    psinfo = 0;
    *ppsinfo = 0;

    if (element)
    {
        SIMEXI_setlasterr("An attempt to get info from an element other than 0 was made.");
        return 0;
    }

    FileIO::Seek(psi->pFileHandle, 0);

    if (!FileIO::Read(psi->pFileHandle, &riffhdr, 12))
    {
        SIMEXI_setlasterr("Couldn't read from file.");
        return 0;
    }

    while (FileIO::Read(psi->pFileHandle, &chunkhdr, 8))
    {
        if (strncmp(chunkhdr.id, "fmt ", 4))
        {
            FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) + GetI(chunkhdr.size, 4));
        }
        else
        {
            FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) - 8);
            break;
        }
    }

    temp_pFileHandle_curpos = FileIO::Tell(psi->pFileHandle);

    if (!FileIO::Read(psi->pFileHandle, &pcmwavehdr, 24))
    {
        SIMEXI_setlasterr("Couldn't read from file.");
        return 0;
    }

    if (GetI(pcmwavehdr.wf.wFormatTag, 2) != 1)
    {
        if (GetI(pcmwavehdr.wf.wFormatTag, 2) == 65534)
        {
            FileIO::Seek(psi->pFileHandle, temp_pFileHandle_curpos);

            if (!FileIO::Read(psi->pFileHandle, &pcmwavehdr, 48))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                return 0;
            }
        }
        else
        {
            SIMEXI_setlasterr("Unrecognized wave format tag.");
            return 0;
        }
    }

    *ppsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));
    psinfo = *ppsinfo;

    if (!psinfo)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for an SINFO structure.");
        return 0;
    }

    SIMEX_defaultsinfo(psinfo);
    psound = psinfo->sound[0];
    psound->numchannels = GetI(pcmwavehdr.wf.nChannels, 2);

    numDataChunks = 0;
    offset = findchunk(psi->pFileHandle, "smpl", &numDataChunks);

    if (offset >= 0)
    {
        FileIO::Seek(psi->pFileHandle, offset);

        if (!FileIO::Read(psi->pFileHandle, &smplhdr, 44))
        {
            SIMEXI_setlasterr("Couldn't read from file.");
            return 0;
        }

        if (GetI(smplhdr.cSampleLoops, 4) > 0)
        {
            if (!FileIO::Read(psi->pFileHandle, &smplloop, 24))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                return 0;
            }

            psound->sustainstart = GetI(smplloop.dwStart, 4);
            psound->sustainend = GetI(smplloop.dwEnd, 4);
        }
    }

    numDataChunks = 0;
    offset = findchunk(psi->pFileHandle, "data", &numDataChunks);

    if (offset < 0)
    {
        SIMEXI_setlasterr("Couldn't find data chunk.");
        return 0;
    }

    FileIO::Seek(psi->pFileHandle, offset);

    if (!FileIO::Read(psi->pFileHandle, &chunkhdr, 8))
    {
        SIMEXI_setlasterr("Couldn't read from file.");
        return 0;
    }

    psound->samplerate = GetI(pcmwavehdr.wf.nSamplesPerSec, 4);
    psound->length = GetI(chunkhdr.size, 4) / psound->numchannels;

    if (GetI(pcmwavehdr.wBitsPerSample, 2) == 8)
    {
        psound->samplerep = 11;
    }
    else if (GetI(pcmwavehdr.wBitsPerSample, 2) == 16)
    {
        psound->samplerep = 0;
        psound->length /= 2;
    }
    else if (GetI(pcmwavehdr.wBitsPerSample, 2) == 16)
    {
        psound->samplerep = 0;
        psound->length /= 2;
    }
    else
    {
        if (GetI(pcmwavehdr.wBitsPerSample, 2) == 24)
        {
            psound->samplerep = 19;
            psound->length /= 3;
        }
        else
        {
            SIMEXI_setlasterr("Unsupported bits per sample in Wave importer.");
            return 0;
        }
    }

    psound->readinfo.sampleoffset = FileIO::Tell(psi->pFileHandle);
    return 1;
}

int readwave(SINSTANCE* psi, SINFO* psinfo, int element)
{
    SAMPLERHDR smplhdr;
    CHUNKHDR chunkhdr;
    SSOUND* psound;
    int ret;
    __int64 offset;
    int numDataChunks;
    SCOMPSTATE* pcompstate[64] = { 0 };
    __int64 sampleoffsets[64] = { 0 };
    SAMPLELOOP smplloop;
    SSOUND tempsound;

    ret = 0;
    element = element;
    psound = psinfo->sound[0];

    if (SIMEXI_allocatecompstate(pcompstate, 64) < 0)
    {
        goto abort;
    }

    if (SIMEXI_allocatetracks(psound) < 0)
    {
        goto abort;
    }

    numDataChunks = 0;
    offset = findchunk(psi->pFileHandle, "smpl", &numDataChunks);
    
    if (offset >= 0)
    {
        FileIO::Seek(psi->pFileHandle, offset);

        if (!FileIO::Read(psi->pFileHandle, &smplhdr, 44))
        {
            SIMEXI_setlasterr("Couldn't read from file.");
            return 0;
        }

        if (GetI(smplhdr.cSampleLoops, 4) > 0)
        {
            if (!FileIO::Read(psi->pFileHandle, &smplloop, 24))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                return 0;
            }

            psound->sustainstart = GetI(smplloop.dwStart, 4);
            psound->sustainend = GetI(smplloop.dwEnd, 4);
        }
    }

    numDataChunks = 0;
    offset = findchunk(psi->pFileHandle, "data", &numDataChunks);

    if (offset < 0)
    {
        SIMEXI_setlasterr("Couldn't find data chunk.");
        return 0;
    }

    FileIO::Seek(psi->pFileHandle, offset);

    if (!FileIO::Read(psi->pFileHandle, &chunkhdr, 8))
    {
        SIMEXI_setlasterr("Couldn't read from file.");
        return 0;
    }

    sampleoffsets[0] = FileIO::Tell(psi->pFileHandle);
    memcpy(&tempsound, psound, sizeof(SSOUND));

    if (!psound->noChannelReordering)
    {
        SIMEXI_remapWaveToITU(psound);
    }

    ret = SIMEXI_importsamples(psi->pFileHandle, sampleoffsets, 0, 0, psound->length, psound->numchannels, psound->samplerep, 0, pcompstate, psound->track, psound);
    memcpy(psound, &tempsound, sizeof(SSOUND));

abort:
    SIMEXI_freecompstate(pcompstate, 64);
    return ret;
}

int writewave(SINSTANCE* pSInstance, SINFO* psinfo, int element)
{
    SAMPLERHDR smplhdr;
    SSOUND* psound;
    CHUNKHDR chunkhdr;
    PCMWAVEEXTENSIBLEHDR pcmwavehdr;
    int ret;
    SCOMPSTATE* pcompstate[64] = { 0 };
    RIFFHDR riffhdr;
    int bitspersample;
    SAMPLELOOP smplloop;
    int trackoffsets[64];
    __int64 length;
    SSOUND tempsound;
    bitspersample = 0;
    ret = 0;

    if (element)
    {
        SIMEXI_setlasterr("WAVE files can only contain element 0.");
        goto abort;
    }

    if (SIMEXI_allocatecompstate(pcompstate, 64) < 0)
    {
        goto abort;
    }

    psound = psinfo->sound[0];
    FileIO::Seek(pSInstance->pFileHandle, 0);

    if (!FileIO::Write(pSInstance->pFileHandle, &riffhdr, 12))
    {
        SIMEXI_setlasterr("Couldn't write to file stream.");
        goto abort;
    }

    PutI(pcmwavehdr.wf.wFormatTag, 1, 2);
    PutI(pcmwavehdr.wf.nChannels, psound->numchannels, 2);
    PutI(pcmwavehdr.wf.nSamplesPerSec, psound->samplerate, 4);

    if (!psound->samplerep)
    {
        bitspersample = 16;
    }
    else if (psound->samplerep == 11)
    {
        bitspersample = 8;
    }
    else
    {
        SIMEXI_setlasterr("Unsupported bits per sample in Wave exporter.");
    }

    PutI(pcmwavehdr.wf.nAvgBytesPerSec, (bitspersample >> 3) * psound->numchannels * psound->samplerate, 4);
    PutI(pcmwavehdr.wf.nBlockAlign, (bitspersample >> 3) * psound->numchannels, 2);
    PutI(pcmwavehdr.wBitsPerSample, bitspersample, 2);

    if (psound->numchannels > 2)
    {
        PutI(pcmwavehdr.wf.wFormatTag, 0xFFFE, 2);
        PutI(pcmwavehdr.wcbSize, 0x16, 2);
        PutI(pcmwavehdr.wValidBitsPerSample, bitspersample, 2);

        if (psound->numchannels == 8)
        {
            PutI(pcmwavehdr.dwChannelMask, 0xFF, 4);
        }
        else if (psound->numchannels == 6)
        {
            PutI(pcmwavehdr.dwChannelMask, 0x3F, 4);
        }
        else if (psound->numchannels == 4)
        {
            PutI(pcmwavehdr.dwChannelMask, 0x33, 4);
        }
        else
        {
            SIMEXI_setlasterr("Invalid number of channels in Wave exporter.  Only 1, 2, 4 or 6 are allowed.");
        }

        if (*(short*)pcmwavehdr.wValidBitsPerSample == bitspersample)
        {
            pcmwavehdr.SubFormat[0] = 1;
            pcmwavehdr.SubFormat[1] = 0;
            pcmwavehdr.SubFormat[2] = 0;
            pcmwavehdr.SubFormat[3] = 0;
            pcmwavehdr.SubFormat[4] = 0;
            pcmwavehdr.SubFormat[5] = 0;
            pcmwavehdr.SubFormat[6] = 16;
            pcmwavehdr.SubFormat[7] = 0;
            pcmwavehdr.SubFormat[8] = 0x80;
            pcmwavehdr.SubFormat[9] = 0;
            pcmwavehdr.SubFormat[10] = 0;
            pcmwavehdr.SubFormat[11] = -86;
            pcmwavehdr.SubFormat[12] = 0;
            pcmwavehdr.SubFormat[13] = 56;
            pcmwavehdr.SubFormat[14] = -101;
            pcmwavehdr.SubFormat[15] = 113;
        }
        else
        {
            pcmwavehdr.SubFormat[0] = 113;
            pcmwavehdr.SubFormat[1] = -101;
            pcmwavehdr.SubFormat[2] = 56;
            pcmwavehdr.SubFormat[3] = 0;
            pcmwavehdr.SubFormat[4] = -86;
            pcmwavehdr.SubFormat[5] = 0;
            pcmwavehdr.SubFormat[6] = 0;
            pcmwavehdr.SubFormat[7] = 0x80;
            pcmwavehdr.SubFormat[8] = 0;
            pcmwavehdr.SubFormat[9] = 16;
            pcmwavehdr.SubFormat[10] = 0;
            pcmwavehdr.SubFormat[11] = 0;
            pcmwavehdr.SubFormat[12] = 0;
            pcmwavehdr.SubFormat[13] = 0;
            pcmwavehdr.SubFormat[14] = 0;
            pcmwavehdr.SubFormat[15] = 1;
        }

        if (!FileIO::Write(pSInstance->pFileHandle, &pcmwavehdr, 48))
        {
            SIMEXI_setlasterr("Couldn't write to file stream.");
            goto abort;
        }
    }
    else
    {
        if (!FileIO::Write(pSInstance->pFileHandle, &pcmwavehdr, 24))
        {
            SIMEXI_setlasterr("Couldn't write to file stream.");
            goto abort;
        }
    }

    if (psound->sustainstart >= 0 && psound->sustainend >= 0)
    {
        strncpy(smplhdr.hdr.id, "smpl", 4);
        PutI(smplhdr.hdr.size, 0x3C, 4);
        PutI(smplhdr.dwManufacturer, 0, 4);
        PutI(smplhdr.dwProduct, 0, 4);
        PutI(smplhdr.dwSamplePeriod, 1.0 / (double)psound->samplerate * 1000000000.0, 4);
        PutI(smplhdr.dwMIDIUnityNote, 0x3C, 4);
        PutI(smplhdr.dwMIDIPitchFraction, 0, 4);
        PutI(smplhdr.dwSMPTEFormat, 0, 4);
        PutI(smplhdr.dwSMPTEOffset, 0, 4);
        PutI(smplhdr.cSampleLoops, 1, 4);
        PutI(smplhdr.cbSamplerData, 0, 4);
        
        if (!FileIO::Write(pSInstance->pFileHandle, &smplhdr, 44))
        {
            SIMEXI_setlasterr("Couldn't write to file stream.");
            goto abort;
        }

        PutI(&smplloop, 0, 4);
        PutI(smplloop.dwType, 0, 4);
        PutI(smplloop.dwStart, psound->sustainstart, 4);
        PutI(smplloop.dwEnd, psound->sustainend, 4);
        PutI(smplloop.dwFraction, 0, 4);
        PutI(smplloop.dwPlayCount, 0, 4);

        if (!FileIO::Write(pSInstance->pFileHandle, &smplloop, 24))
        {
            SIMEXI_setlasterr("Couldn't write to file stream.");
            goto abort;
        }
    }

    strncpy(chunkhdr.id, "data", 4);
    PutI(chunkhdr.size, (bitspersample >> 3) * psound->numchannels * psound->length, 4);

    if (!FileIO::Write(pSInstance->pFileHandle, &chunkhdr, 8))
    {
        SIMEXI_setlasterr("Couldn't write to file stream.");
        goto abort;
    }

    memcpy(&tempsound, psound, sizeof(SSOUND));

    if (!psound->noChannelReordering)
    {
        SIMEXI_remapITUToWave(psound);
    }

    ret = SIMEXI_exportsamplesfile(pSInstance, pSInstance->pFileHandle, 0, psound->length, 0, pcompstate, trackoffsets, psound);
    memcpy(psound, &tempsound, sizeof(SSOUND));

    if (!ret)
    {
        goto abort;
    }

    length = FileIO::Tell(pSInstance->pFileHandle);
    FileIO::Seek(pSInstance->pFileHandle, 0);
    strncpy(riffhdr.hdr.id, "RIFF", 4);
    PutI(riffhdr.hdr.size, length - 8, 4);
    strncpy(riffhdr.fcctype, "WAVE", 4);

    if (!FileIO::Write(pSInstance->pFileHandle, &riffhdr, 12))
    {
        SIMEXI_setlasterr("Couldn't write to file stream.");
        ret = 0;
        goto abort;
    }

    strncpy(chunkhdr.id, "fmt ", 4);

    if (psound->numchannels > 2)
    {
        PutI(chunkhdr.size, 0x28, 4);
    }
    else
    {
        PutI(chunkhdr.size, 0x10, 4);
    }

    if (!FileIO::Write(pSInstance->pFileHandle, &chunkhdr, 8))
    {
        SIMEXI_setlasterr("Couldn't write to file stream.");
        ret = 0;
        goto abort;
    }

abort:
    SIMEXI_freecompstate(pcompstate, 64);
    return ret;
}
