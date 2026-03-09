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
#include <string.h>

const char *samplerepswitches[] = {
"s16l_int",
"s16b_int",
0,
0,
0,
0,
0,
0,
0,
0,
"eaxa_blk",
"u8_int",
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
"xas_int",
"ealayer3_int",
"ealayer3pcm_int",
"ealayer3spike_int"
};

const char *SIMEX_getsamplerepswitch(int samplerep)
{
    if (samplerep < 0 || samplerep >= 33)
    {
        return "Unknown";
    }
    
    return samplerepswitches[samplerep];
}

int SIMEX_samplerepexpsupported(int fileFormat, int sampleRep)
{
    SABOUT sAbout;
    int i = 0;

    if (!SIMEX_about(fileFormat, &sAbout))
    {
        return 0;
    }

    while (sAbout.exp.samplereps[i] >= 0)
    {
        if (sampleRep == sAbout.exp.samplereps[i])
        {
            return 1;
        }

        i++;
    }

    return 0;
}

int SIMEXI_rendermodesupported(int fileFormat, SSOUND* pSSound)
{
    SABOUT sAbout;

    if (!SIMEX_about(fileFormat, &sAbout))
    {
        return 0;
    }

    int modes = sAbout.playlocdefault + sAbout.playlocspu + sAbout.playlocmaincpu + sAbout.playlociopcpu + sAbout.playlocds2dhw + sAbout.playlocds3dhw + sAbout.playlocdsp + sAbout.playlocram + sAbout.playlocstream + sAbout.playlocgigasample;

    if (modes > 1)
    {
        if (((pSSound->playloc & 8) != 0 && !sAbout.playlocspu) || ((pSSound->playloc & 4) != 0 && !sAbout.playlocmaincpu) || ((pSSound->playloc & 0x100) != 0 && !sAbout.playlociopcpu) || ((pSSound->playloc & 0x200) != 0 && !sAbout.playlocdsp) || ((pSSound->playloc & 0x400) != 0 && !sAbout.playlocds2dhw) || ((pSSound->playloc & 0x10) != 0 && !sAbout.playlocds3dhw) || ((pSSound->playloc & 0x800) != 0 && !sAbout.playlocram) || ((pSSound->playloc & 0x1000) != 0 && !sAbout.playlocstream) || ((pSSound->playloc & 0x2000) != 0 && !sAbout.playlocgigasample))
        {
            SIMEXI_setlasterr("The specified playloc is not supported by this exporter.");
            return 0;
        }
    }

    return 1;
}

void SIMEX_defaultssound(SSOUND* psound)
{
    memset(psound, 0, sizeof(SSOUND));
    psound->readinfo.sampleoffset = -1;
    psound->velmax = 127;
    psound->keymax = 127;
    psound->keybase = 60;
    psound->pan = 64;
    psound->panmult = 1;
    psound->vol = 127;
    psound->numenvelopes = 1;
    psound->releaseenvelope = -1;
    psound->initialenvelopevol = 127;
    psound->bend = 64;
    psound->samplerep = -1;
    psound->truncateloops = 1;
    psound->sustainstart = -1;
    psound->sustainend = -1;
    psound->envelope[0].duration = 0x7FFFFFFF;
    psound->bitrate = 50;
    psound->envelope[0].targetvol = 127;

    for (int i = 0; i < 6; i++)
    {
        psound->azimuth[i] = -1;
    }

    psound->loopoffset[0] = 0;
    psound->loopoffset[1] = 0;
    psound->loopoffset[2] = 0;
    psound->loopoffset[3] = 0;
    psound->loopoffset[4] = 0;
    psound->loopoffset[5] = 0;
    psound->impResponseInfo.samplesPerFFTBlock = 1024;
    psound->impResponseInfo.percentHighFrequencyCut = 0;
    psound->noChannelReordering = 0;
}

int SIMEXI_addssound(SINFO* psinfo, int index)
{
    if (!psinfo->sound[index])
    {
        psinfo->sound[index] = (SSOUND*)Allocator::Alloc(sizeof(SSOUND));

        if (!psinfo->sound[index])
        {
            SIMEXI_setlasterr("Not enough memory for SSOUND allocation.");
            return -1;
        }

        SIMEX_defaultssound(psinfo->sound[psinfo->numsounds]);
    }

    return 0;
}

void SIMEX_defaultsinfo(SINFO* psinfo)
{
    memset(psinfo, 0, sizeof(SINFO));
    psinfo->cps = 15.0f;
    psinfo->iscpsdefault = 1;
    psinfo->numsounds = 1;
    psinfo->sound[0] = (SSOUND*)Allocator::Alloc(sizeof(SSOUND));
    SIMEX_defaultssound(psinfo->sound[0]);

    for (int i = 1; i < 32; i++)
    {
        psinfo->sound[i] = 0;
    }
}

int PlatformToCodecVer(int platform, unsigned char platformver, signed char samplerep)
{
    int codecVersion = 0;

    if (samplerep == 10 || samplerep == 4 || samplerep == 22)
    {
        if (platform == 8)
        {
            if (platformver > 2)
            {
                codecVersion = 1;
            }
        }
        else if (!platform)
        {
            if (platformver > 1)
            {
                codecVersion = 1;
            }
        }
        else if (platform == 7)
        {
            if (platformver > 2)
            {
                codecVersion = 1;
            }
        }
        else if (platform == 5)
        {
            if (platformver > 2)
            {
                codecVersion = 1;
            }
        }
        else if (platform == 6)
        {
            if (platformver > 2)
            {
                codecVersion = 1;
            }
        }
        else if (platform == 9)
        {
            codecVersion = 1;
        }
        else if (platform == 14)
        {
            codecVersion = 1;
        }
        else if (platform == 10)
        {
            codecVersion = 1;
        }
    }

    return codecVersion;
}

int SIMEX_copysinfo(SINFO* pdstsinfo, SINFO* psrcsinfo)
{
    int i;
    int j;
    int k;
    SSOUND* psrcsound;
    SSOUND* pdstsound;
    *pdstsinfo = *psrcsinfo;

    for (i = 0; i < psrcsinfo->numsounds; i++)
    {
        psrcsound = psrcsinfo->sound[i];
        pdstsinfo->sound[i] = (SSOUND*)Allocator::Alloc(sizeof(SSOUND));
        pdstsound = pdstsinfo->sound[i];
        memcpy(pdstsound, psrcsound, sizeof(SSOUND));

        for (j = 0; j < 4; j++)
        {
            pdstsound->userdatasize[j] = psrcsound->userdatasize[j];

            if (psrcsound->userdatasize[j])
            {
                pdstsound->puserdata[j] = Allocator::Alloc(psrcsound->userdatasize[j]);
                
                if (!pdstsound->puserdata[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memcpy(pdstsound->puserdata[j], psrcsound->puserdata[j], psrcsound->userdatasize[j]);
            }
        }

        for (j = 0; j < 200; j++)
        {
            if (psrcsound->pmarkerchunklist[j])
            {
                pdstsound->pmarkerchunklist[j] = (MARKCHUNK*)Allocator::Alloc(sizeof(MARKCHUNK));

                if (!pdstsound->pmarkerchunklist[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memset(pdstsound->pmarkerchunklist[j], 0, sizeof(MARKCHUNK));
                pdstsound->pmarkerchunklist[j]->nummarkers = psrcsound->pmarkerchunklist[j]->nummarkers;

                for (k = 0; k < psrcsound->pmarkerchunklist[j]->nummarkers; k++)
                {
                    if (psrcsound->pmarkerchunklist[j]->marks[k])
                    {
                        pdstsound->pmarkerchunklist[j]->marks[k] = (MARK*)Allocator::Alloc(sizeof(MARK));

                        if (!pdstsound->pmarkerchunklist[j]->marks[k])
                        {
                            SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                            return 0;
                        }

                        pdstsound->pmarkerchunklist[j]->marks[k]->id = psrcsound->pmarkerchunklist[j]->marks[k]->id;
                        pdstsound->pmarkerchunklist[j]->marks[k]->position = psrcsound->pmarkerchunklist[j]->marks[k]->position;
                        pdstsound->pmarkerchunklist[j]->marks[k]->length = psrcsound->pmarkerchunklist[j]->marks[k]->length;

                        if (psrcsound->pmarkerchunklist[j]->marks[k]->length)
                        {
                            pdstsound->pmarkerchunklist[j]->marks[k]->string = (char*)Allocator::Alloc(psrcsound->pmarkerchunklist[j]->marks[k]->length);
                            if (!pdstsound->pmarkerchunklist[j]->marks[k]->string)
                            {
                                SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                                return 0;
                            }

                            memcpy(pdstsound->pmarkerchunklist[j]->marks[k]->string, psrcsound->pmarkerchunklist[j]->marks[k]->string,psrcsound->pmarkerchunklist[j]->marks[k]->length);
                        }
                    }
                    else
                    {
                        pdstsound->pmarkerchunklist[j]->marks[k] = 0;
                    }
                }
            }
            else
            {
                pdstsound->pmarkerchunklist[j] = 0;
            }
        }

        if (psrcsound->pvoltable)
        {
            pdstsound->pvoltable = (SSCALINGTABLE*)Allocator::Alloc(sizeof(SSCALINGTABLE));

            if (!pdstsound->pvoltable)
            {
                SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                return 0;
            }

            memcpy(pdstsound->pvoltable, psrcsound->pvoltable, sizeof(SSCALINGTABLE));
        }

        if (psrcsound->pbendtable)
        {
            pdstsound->pbendtable = (SSCALINGTABLE*)Allocator::Alloc(sizeof(SSCALINGTABLE));

            if (!pdstsound->pbendtable)
            {
                SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                return 0;
            }

            memcpy(pdstsound->pbendtable, psrcsound->pbendtable, sizeof(SSCALINGTABLE));
        }

        if (psrcsound->pvollfo)
        {
            pdstsound->pvollfo = (SSCALINGTABLE*)Allocator::Alloc(sizeof(SSCALINGTABLE));

            if (!pdstsound->pvollfo)
            {
                SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                return 0;
            }

            memcpy(pdstsound->pvollfo, psrcsound->pvollfo, sizeof(SSCALINGTABLE));
        }

        if (psrcsound->ppitchlfo)
        {
            pdstsound->ppitchlfo = (SSCALINGTABLE*)Allocator::Alloc(sizeof(SSCALINGTABLE));
            if (!pdstsound->ppitchlfo)
            {
                SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                return 0;
            }
            memcpy(pdstsound->ppitchlfo, psrcsound->ppitchlfo, sizeof(SSCALINGTABLE));
        }

        for (j = 0; j < psrcsound->numchannels; j++)
        {
            if (psrcsound->track[j])
            {
                pdstsound->track[j] = (short*)Allocator::Alloc(2 * psrcsound->length);

                if (!pdstsound->track[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memcpy(pdstsound->track[j], psrcsound->track[j], 2 * psrcsound->length);
            }
        }

        for (j = 0; j < 6; j++)
        {
            if (psrcsound->ptssidebanddata[j])
            {
                pdstsound->ptssidebanddata[j] = (unsigned char*)Allocator::Alloc(psrcsound->tssidebandsize);

                if (!pdstsound->ptssidebanddata[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memcpy(pdstsound->ptssidebanddata[j], psrcsound->ptssidebanddata[j], psrcsound->tssidebandsize);
            }

            if (psrcsound->ppropcodebook[j])
            {
                pdstsound->ppropcodebook[j] = Allocator::Alloc(psrcsound->propcodebooklen[j]);

                if (!pdstsound->ppropcodebook[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memcpy(pdstsound->ppropcodebook[j], psrcsound->ppropcodebook[j], psrcsound->propcodebooklen[j]);
            }

            if (psrcsound->pproploopstate[j])
            {
                pdstsound->pproploopstate[j] = Allocator::Alloc(psrcsound->proploopstatelen[j]);

                if (!pdstsound->pproploopstate[j])
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for copysinfo.\n");
                    return 0;
                }

                memcpy(pdstsound->pproploopstate[j], psrcsound->pproploopstate[j], psrcsound->proploopstatelen[j]);
            }
        }
    }

    return 1;
}

int SIMEX_freessound(SINFO* psinfo, int index)
{
    int i;
    int j;
    SSOUND *pss = psinfo->sound[index];

    if (!pss)
    {
        return 0;
    }

    if (pss->pvoltable)
    {
        Allocator::Free(pss->pvoltable);
    }

    if (pss->pbendtable)
    {
        Allocator::Free(pss->pbendtable);
    }

    if (pss->pvollfo)
    {
        Allocator::Free(pss->pvollfo);
    }

    if (pss->ppitchlfo)
    {
        Allocator::Free(pss->ppitchlfo);
    }

    for (i = 0; i < 4; i++)
    {
        if (pss->puserdata[i])
        {
            Allocator::Free(pss->puserdata[i]);
        }
    }

    for (i = 0; i < pss->markerchunkcount; i++)
    {
        if (pss->pmarkerchunklist[i])
        {
            for (j = 0; j < pss->pmarkerchunklist[i]->nummarkers; j++)
            {
                if (pss->pmarkerchunklist[i]->marks[j])
                {
                    if (pss->pmarkerchunklist[i]->marks[j]->length > 0)
                    {
                        Allocator::Free(pss->pmarkerchunklist[i]->marks[j]->string);
                    }

                    Allocator::Free(pss->pmarkerchunklist[i]->marks[j]);
                    pss->pmarkerchunklist[i]->marks[j] = 0;
                }
            }

            Allocator::Free(pss->pmarkerchunklist[i]);
        }
    }

    for (i = 0; i < pss->numchannels; i++)
    {
        if (pss->track[i])
        {
            Allocator::Free(pss->track[i]);
        }
    }

    for (i = 0; i < 6; i++)
    {
        if (pss->ptssidebanddata[i])
        {
            Allocator::Free(pss->ptssidebanddata[i]);
        }

        if (pss->ppropcodebook[i])
        {
            Allocator::Free(pss->ppropcodebook[i]);
        }

        if (pss->pproploopstate[i])
        {
            Allocator::Free(pss->pproploopstate[i]);
        }
    }

    Allocator::Free(pss);

    psinfo->sound[index] = 0;
    psinfo->numsounds--;
    int offset = 0;

    for (i = 0; i < psinfo->numsounds; i++)
    {
        if (!psinfo->sound[i])
        {
            while (!psinfo->sound[i + offset])
            {
                offset++;
            }

            psinfo->sound[i] = psinfo->sound[i + offset];
            psinfo->sound[i + offset] = 0;
        }
    }

    return 1;
}

int SIMEX_freesinfo(SINFO* psinfo)
{
    if (!psinfo)
    {
        return 0;
    }

    while (psinfo->numsounds)
    {
        if (!SIMEX_freessound(psinfo, psinfo->numsounds - 1))
        {
            return 0;
        }
    }

    Allocator::Free(psinfo);
    return 1;
}

int SIMEXI_allocatetracks(SSOUND* psound)
{
    int i;

    for (i = 0; i < psound->numchannels; i++)
    {
        psound->track[i] = (short*)Allocator::Alloc(2 * psound->length);
        
        if (!psound->track[i])
        {
            for (i = i - 1; i >= 0; i--)
            {
                Allocator::Free(psound->track[i]);
            }

            SIMEXI_setlasterr("Couldn't allocate memory for audio samples.");
            return -1;
        }
    }

    return 0;
}

void SIMEXI_resetcompstate(SCOMPSTATE* pstate)
{
    pstate->sample1 = 0;
    pstate->stepindex1 = 0;
    pstate->sample2 = 0;
    pstate->sustainstart = -1;
    pstate->sustainend = -1;
    pstate->diff1 = 0.0;
    pstate->diff2 = 0.0;
    pstate->isstream = 0;
    pstate->samplerate = 0;
    pstate->bufframes = 0;
    pstate->loopoffset = -1;
    pstate->pstate = 0;
}

int SIMEXI_allocatecompstate(SCOMPSTATE* pcompstate[], int numTracks)
{
    int i;

    for (i = 0; i < numTracks; i++)
    {
        pcompstate[i] = (SCOMPSTATE*)Allocator::Alloc(sizeof(SCOMPSTATE));

        if (!pcompstate[i])
        {
            for (i = i - 1; i >= 0; i--)
            {
                Allocator::Free(pcompstate[i]);
            }

            SIMEXI_setlasterr("Couldn't allocate memory for compression state info.");
            return -1;
        }

        SIMEXI_resetcompstate(pcompstate[i]);
    }

    return 0;
}

void SIMEXI_freecompstate(SCOMPSTATE* pcompstate[], int numTracks)
{
    int i;

    for (i = 0; i < numTracks; i++)
    {
        if (pcompstate[i])
        {
            Allocator::Free(pcompstate[i]);
        }
    }
}

int puttagmv(FileHandle* pFileHandle, unsigned char tag, int defaultval, int value)
{
    unsigned char sizeofval;
    unsigned char dstval;
    unsigned int pval;

    if (defaultval == value)
    {
        return 0;
    }

    if (!value)
    {
        sizeofval = 0;
    }
    else if (value <= 127 && value >= -128)
    {
        sizeofval = 1;
    }
    else if (value <= 32767 && value >= -32768)
    {
        sizeofval = 2;
    }
    else if (value <= 8388352 && value >= -8388608)
    {
        sizeofval = 3;
    }
    else
    {
        sizeofval = 4;
    }

    FileIO::Write(pFileHandle, &tag, 1);
    FileIO::Write(pFileHandle, &sizeofval, 1);
    pval = value;

    while (sizeofval)
    {
        sizeofval--;
        dstval = pval >> (8 * sizeofval);
        FileIO::Write(pFileHandle, &dstval, 1);
    }

    return 0;
}

int puttagm(FileHandle* pFileHandle, unsigned char tag, int defaultval, int value, int sizeofval)
{
    unsigned char charsizeofval;
    unsigned char dstval;
    unsigned int pval;

    if (defaultval == value)
    {
        return 0;
    }

    charsizeofval = sizeofval;
    FileIO::Write(pFileHandle, &tag, 1);
    FileIO::Write(pFileHandle, &charsizeofval, 1);
    pval = value;

    while (sizeofval)
    {
        sizeofval--;
        dstval = pval >> (8 * sizeofval);
        FileIO::Write(pFileHandle, &dstval, 1);
    }

    return 0;
}

int puttagdata(FileHandle* pFileHandle, unsigned char tag, void* pdata, int datasize)
{
    unsigned char charsizeofval;
    unsigned int pval;
    int i;

    if (!pdata || !datasize)
    {
        return 0;
    }

    FileIO::Write(pFileHandle, &tag, 1);

    if (datasize < 255)
    {
        charsizeofval = datasize;
        FileIO::Write(pFileHandle, &charsizeofval, 1);
    }
    else
    {
        charsizeofval = -1;
        FileIO::Write(pFileHandle, &charsizeofval, 1);
        pval = datasize;

        for (i = 3; i >= 0; i--)
        {
            charsizeofval = pval >> (8 * i);
            FileIO::Write(pFileHandle, &charsizeofval, 1);
        }
    }

    FileIO::Write(pFileHandle, pdata, datasize);
    return 0;
}

int putmarker(FileHandle* pFileHandle, unsigned char marker)
{
    FileIO::Write(pFileHandle, &marker, 1);
    return 0;
}

void(*SIMEXprogresscb)(int);

void SIMEXI_progresscb(int percentdone)
{
    if (SIMEXprogresscb)
    {
        SIMEXprogresscb(percentdone);
    }
}

void(*SIMEXwarningcb)(const char*);

void SIMEXI_warningcb(const char* format, ...)
{
    char warningbuf[512];
    va_list val;
    
    if (SIMEXwarningcb)
    {
        if (!format)
        {
            return;
        }

        va_start(val, format);
        vsprintf(warningbuf, format, val);
        va_end(val);
        SIMEXwarningcb(warningbuf);
    }
}
