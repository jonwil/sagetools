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
#include "cmn\sbnkutil.h"
#include "coda\include\coda.h"
#include "cmn\isimex.h"
#include "cmn\fileio.h"

struct TAGGEDPATCH
{
    short id;
    unsigned char platform;
    unsigned char flags;
    int hdrsize;
};

int SIMEXI_readgccodebook(FileHandle* pgs, SSOUND* pss, int bigendian, int channel)
{
    short* pshort;
    int i;

    pshort = (short*)Allocator::Alloc(33);

    if (!pshort)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for code book data.");
        return -1;
    }

    FileIO::Read(pgs, pshort, 33);

    if (!pshort)
    {
        SIMEXI_setlasterr("Couldn't read code book data.");
        return -1;
    }

    for (i = 0; i < 16; i++)
    {
        if (bigendian)
        {
            pshort[i] = GetM(&pshort[i], 2);
        }
        else
        {
            pshort[i] = GetI(&pshort[i], 2);
        }
    }

    pss->ppropcodebook[channel] = pshort;
    pss->propcodebooklen[channel] = 33;
    return 0;
}

int SIMEXI_readTsSidebandData(FileHandle* pgs, SSOUND* pss, int channel, int size)
{
    unsigned char* pchar = (unsigned char *)Allocator::Alloc(size);

    if (!pchar)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for ts sideband data.");
        return -1;
    }

    FileIO::Read(pgs, pchar, size);

    if (!pchar)
    {
        SIMEXI_setlasterr("Couldn't read ts sideband data.");
        return -1;
    }

    pss->ptssidebanddata[channel] = pchar;
    pss->tssidebandsize = size;
    return 0;
}

void SIMEXI_aligntag(FileHandle* pgs, int datasize, int alignment)
{
    int alignpad;
    unsigned char tag;
    int taghdrsize;

    taghdrsize = 2;
    tag = -4;

    if (datasize >= 255)
    {
        taghdrsize += 4;
    }

    alignpad = alignment - taghdrsize % alignment;

    while (FileIO::Tell(pgs) % alignment != alignpad)
    {
        FileIO::Write(pgs, &tag, 1);
    }
}

int gettagdata(void* psrc, int bytes)
{
    int origbytes;
    unsigned char* s;
    int value;

    s = (unsigned char*)psrc;
    value = 0;
    origbytes = bytes;

    while (bytes--)
    {
        value = (value << 8) + *s;
        s++;
    }

    if (origbytes == 1 && value > 127)
    {
        value -= 256;
    }
    else if (origbytes == 2 && value > 0x7FFF)
    {
        value -= 0x10000;
    }
    else if (origbytes == 3 && value > 0x7FFFFF)
    {
        value -= 0x1000000;
    }

    return value;
}

int resettimbre(int sndtags[], int platform)
{
    sndtags[1] = 0;
    sndtags[2] = 127;
    sndtags[3] = 0;
    sndtags[4] = 127;
    sndtags[5] = 50;
    sndtags[6] = 0;
    sndtags[7] = 60;
    sndtags[8] = -1;
    sndtags[9] = 1;
    sndtags[10] = 0;
    sndtags[12] = 64;
    sndtags[13] = 0;
    sndtags[14] = 127;
    sndtags[15] = 0;
    sndtags[16] = 0;
    sndtags[17] = 0;
    sndtags[18] = 0;
    sndtags[23] = 0;
    sndtags[25] = 0;
    sndtags[28] = 127;
    sndtags[29] = 0;
    sndtags[30] = 0;
    sndtags[31] = 0;
    sndtags[32] = 0;
    sndtags[33] = 0;
    sndtags[35] = 0;
    sndtags[34] = 0;
    sndtags[37] = 1;
    sndtags[156] = 0;
    sndtags[157] = 0;
    sndtags[158] = 0;
    sndtags[159] = 0;
    sndtags[166] = 0;
    sndtags[167] = 0;
    sndtags[136] = 0;
    sndtags[137] = 0;
    sndtags[148] = 0;
    sndtags[149] = 0;
    sndtags[162] = 0;
    sndtags[163] = 0;

    if (!platform || platform == 11)
    {
        sndtags[128] = 0;
        sndtags[130] = 1;
        sndtags[132] = 22050;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 0;
        sndtags[160] = 10;
    }
    else if (platform == 7)
    {
        sndtags[128] = 2;
        sndtags[130] = 1;
        sndtags[132] = 24000;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 0;
        sndtags[160] = 8;
    }
    else if (platform == 5)
    {
        sndtags[128] = 0;
        sndtags[130] = 1;
        sndtags[132] = 22050;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 8;
        sndtags[160] = 5;
    }
    else if (platform == 6)
    {
        sndtags[128] = 2;
        sndtags[130] = 1;
        sndtags[132] = 24000;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 512;
        sndtags[160] = 7;
    }
    else if (platform == 8)
    {
        sndtags[130] = 1;
        sndtags[132] = 48000;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[160] = 10;
        sndtags[140] = 0;
        sndtags[0] = 83886080;
    }
    else if (platform == 9 || platform == 12)
    {
        sndtags[128] = 1;
        sndtags[130] = 1;
        sndtags[132] = 44100;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 0;
        sndtags[160] = 10;
    }
    else if (platform == 10 || platform == 13)
    {
        sndtags[128] = 0;
        sndtags[130] = 1;
        sndtags[132] = 22050;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 4;
        sndtags[160] = 10;
    }
    else if (platform == 14 || platform == 15)
    {
        sndtags[128] = 0;
        sndtags[130] = 1;
        sndtags[132] = 44100;
        sndtags[133] = 0;
        sndtags[134] = -1;
        sndtags[135] = -1;
        sndtags[140] = 0;
        sndtags[160] = 10;
    }
    else
    {
        SIMEXI_setlasterr("Platform not supported in resettimbre.");
        return 0;
    }

    return 1;
}

static const int schancfg3d[6][6] = { {0, 0, 0, 0, 0, 0}, {0xE000, 0x2000, 0, 0, 0, 0}, {0xE000, 0x2000, 0x8000, 0, 0, 0}, {0xE000, 0x2000, 0xA000, 0x6000, 0, 0}, {0xE000, 0, 0x2000, 0xA000, 0x6000, 0}, {0xE000, 0, 0x2000, 0xA000, 0x6000, 0} };

int SIMEXI_copytimbre(SINSTANCE* psi, SSOUND* pss, int* sndtags, __int64 envelopeoffset, int platform, int bigendian)
{
    __int64 savedfileoffset;
    MPEGAUDIOHDR mah = { 0 };
    unsigned int mpegheader;
    int i;
    SENVELOPE envelope;

    pss->velmin = sndtags[1];
    pss->velmax = sndtags[2];
    pss->keymin = sndtags[3];
    pss->keymax = sndtags[4];
    pss->priority = sndtags[6];
    pss->keybase = sndtags[7];
    pss->releaseenvelope = sndtags[8];
    pss->numenvelopes = sndtags[9];
    pss->bendrange = sndtags[10];
    pss->pan = sndtags[12];
    pss->randpan = sndtags[13];
    pss->panmult = sndtags[37];
    pss->vol = sndtags[14];
    pss->randvol = sndtags[15];
    pss->detune = sndtags[16];
    pss->randdetune = sndtags[17];
    pss->initialenvelopevol = sndtags[28];
    pss->vollfolength = sndtags[30];
    pss->vollforandstart = sndtags[31];
    pss->pitchlfolength = sndtags[33];
    pss->pitchlforandstart = sndtags[35];
    pss->pitchlfodepth = sndtags[34];
    pss->platformver = sndtags[128];
    pss->samplerep = sndtags[160];
    pss->numchannels = sndtags[130];
    pss->bitrate = sndtags[5];
    pss->samplerate = sndtags[132];
    pss->length = sndtags[133];
    pss->sustainstart = sndtags[134];
    pss->sustainend = sndtags[135];
    pss->loopoffset[0] = sndtags[26];
    pss->loopoffset[1] = sndtags[38];
    pss->loopoffset[2] = sndtags[39];
    pss->loopoffset[3] = sndtags[40];
    pss->loopoffset[4] = sndtags[41];
    pss->loopoffset[5] = sndtags[42];
    pss->readinfo.sampleoffset = sndtags[136];
    pss->playloc = sndtags[140] & 0x71C;

    if (envelopeoffset)
    {
        savedfileoffset = FileIO::Tell(psi->pFileHandle);

        if (!FileIO::Seek(psi->pFileHandle, envelopeoffset))
        {
            SIMEXI_setlasterr("Couldn't seek to envelope offset.");
            return 0;
        }

        if (pss->numenvelopes > 8)
        {
            SIMEXI_setlasterr("Envelope array to small for storage.");
            return 0;
        }

        for (i = 0; i < pss->numenvelopes; i++)
        {
            if (!FileIO::Read(psi->pFileHandle, &envelope, 8))
            {
                SIMEXI_setlasterr("Couldn't read envelope.");
                return 0;
            }

            pss->envelope[i].duration = bigendian ? GetM(&envelope.duration, 4) : GetI(&envelope.duration, 4);
            pss->envelope[i].targetvol = bigendian ? GetM(&envelope.targetvol, 4) : GetI(&envelope.targetvol, 4);
        }

        FileIO::Seek(psi->pFileHandle, savedfileoffset);
    }

    if (pss->samplerep == 14 || pss->samplerep == 15 || pss->samplerep == 16)
    {
        savedfileoffset = FileIO::Tell(psi->pFileHandle);

        if (pss->readinfo.sampleoffset)
        {
            FileIO::Seek(psi->pFileHandle, pss->readinfo.sampleoffset);
            FileIO::Read(psi->pFileHandle, &mpegheader, 4);
            SIMEXI_mpegparseheader(GetM(&mpegheader, 4), &mah);
            pss->bitrate = 1000 * mah.bitrate;
        }
        else
        {
            FileIO::Seek(psi->pFileHandle, (FileIO::Tell(psi->pFileHandle) + 3) & 0x7FFFFFFC);

            for (i = 0; i < 100000; i++)
            {
                if (FileIO::Read(psi->pFileHandle, &mpegheader, 4) == 4)
                {
                    if (SIMEXI_mpegparseheader(GetM(&mpegheader, 4), &mah))
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            if (i < 100000)
            {
                pss->bitrate = 1000 * mah.bitrate;
            }
        }

        FileIO::Seek(psi->pFileHandle, savedfileoffset);
    }

    if ((psi->fileformat == 19 || psi->fileformat == 37) && pss->samplerep == 5)
    {
        if (pss->sustainend > 0)
        {
            pss->length -= 28;
        }
        else
        {
            pss->length -= 56;
        }
    }

    if (sndtags[156] || sndtags[157] || sndtags[158] || sndtags[159] || sndtags[166] || sndtags[167])
    {
        pss->azimuth[0] = schancfg3d[pss->numchannels - 1][0] + sndtags[156];
        pss->azimuth[1] = schancfg3d[pss->numchannels - 1][1] + sndtags[157];
        pss->azimuth[2] = schancfg3d[pss->numchannels - 1][2] + sndtags[158];
        pss->azimuth[3] = schancfg3d[pss->numchannels - 1][3] + sndtags[159];
        pss->azimuth[4] = schancfg3d[pss->numchannels - 1][4] + sndtags[166];
        pss->azimuth[5] = schancfg3d[pss->numchannels - 1][5] + sndtags[167];
    }

    return 1;
}

int readtable(FileHandle* pgs, __int64 offset, SSCALINGTABLE** pptable)
{
    __int64 savedfileoffset;

    if (*pptable)
    {
        SIMEXI_setlasterr("Scaling table is already allocated.");
        return 0;
    }

    savedfileoffset = FileIO::Tell(pgs);

    if (!FileIO::Seek(pgs, savedfileoffset + offset - 4))
    {
        SIMEXI_setlasterr("Couldn't seek to scaling table.");
        return 0;
    }

    *pptable = (SSCALINGTABLE*)Allocator::Alloc(128);

    if (!*pptable)
    {
        SIMEXI_setlasterr("Couldn't allocate scaling table.");
        return 0;
    }

    if (!FileIO::Read(pgs, *pptable, 128))
    {
        SIMEXI_setlasterr("Couldn't read scaling table.");
        return 0;
    }

    FileIO::Seek(pgs, savedfileoffset);
    return 1;
}

int iinfopatch(SINSTANCE* psi, SINFO** ppsinfo, int platform, int bigendian, __int64 patchoffset)
{
    __int64 envelopeoffset;
    unsigned int data;
    SINFO* psinfo;
    char buf[4];
    int sndtags[256];
    unsigned int tagsize;
    SSOUND* pss;
    TAGGEDPATCH patch;
    unsigned char tag;
    unsigned char tagsize1;
    char* ptemp;

    psinfo = 0;
    pss = 0;
    envelopeoffset = 0;
    *ppsinfo = 0;
    sndtags[0] = 251658240;
    sndtags[36] = 0;

    if (!FileIO::Seek(psi->pFileHandle, patchoffset))
    {
        return 0;
    }

    if (!FileIO::Read(psi->pFileHandle, &patch, 8))
    {
        return 0;
    }

    if ((patch.flags & 2) == 0)
    {
        if (!FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) - 4))
        {
            return 0;
        }
    }

    if (!resettimbre(sndtags, platform))
    {
        goto abort;
    }

    *ppsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));
    psinfo = *ppsinfo;

    if (!psinfo)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for SINFO structure.");
        goto abort;
    }

    SIMEX_defaultsinfo(psinfo);
    pss = psinfo->sound[psinfo->numsounds - 1];

    if (!FileIO::Read(psi->pFileHandle, &tag, 1))
    {
        SIMEXI_setlasterr("Couldn't read from file.");
        goto abort;
    }

    while (tag != 255)
    {
        if (tag == 254)
        {
            if (!SIMEXI_copytimbre(psi, pss, sndtags, envelopeoffset, platform, bigendian))
            {
                goto abort;
            }

            SIMEXI_addssound(psinfo, psinfo->numsounds);
            pss = psinfo->sound[psinfo->numsounds++];

            if (!resettimbre(sndtags, platform))
            {
                goto abort;
            }

            if (!FileIO::Read(psi->pFileHandle, &tag, 1))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                goto abort;
            }

            continue;
        }
        else if (tag == 252 || tag == 253)
        {
            if (!FileIO::Read(psi->pFileHandle, &tag, 1))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                goto abort;
            }

            continue;
        }

        if (!FileIO::Read(psi->pFileHandle, &tagsize1, 1))
        {
            SIMEXI_setlasterr("Couldn't read from file.");
            goto abort;
        }

        tagsize = tagsize1;

        if (tagsize == 255)
        {
            if (!FileIO::Read(psi->pFileHandle, buf, 4))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                goto abort;
            }

            tagsize = gettagdata(buf, 4);
        }

        if (tag == 20)
        {
            ptemp = (char*)Allocator::Alloc(tagsize);

            if (!ptemp)
            {
                SIMEXI_setlasterr("Couldn't allocate memory for user data.");
                return 0;
            }

            FileIO::Read(psi->pFileHandle, ptemp, tagsize);

            if (!ptemp)
            {
                SIMEXI_setlasterr("Couldn't read user data.");
                return 0;
            }

            SIMEXFILTERPARAM sfp;
            sfp.datasize = tagsize;
            sfp.pdata = ptemp;
            SIMEX_filterssound(pss, 92, &sfp);
            Allocator::Free(ptemp);
            goto nexttag;
        }
        
        if (tag == 143)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 0) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 144)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 1) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 145)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 2) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 171)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 3) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 172)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 4) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 173)
        {
            if (SIMEXI_readgccodebook(psi->pFileHandle, pss, bigendian, 5) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 152)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 0, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 153)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 1, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 154)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 2, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 155)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 3, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 164)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 4, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }
        
        if (tag == 165)
        {
            if (SIMEXI_readTsSidebandData(psi->pFileHandle, pss, 5, tagsize) < 0)
            {
                return 0;
            }

            goto nexttag;
        }

        if (!tagsize)
        {
            sndtags[tag] = 0;
        }
        else if (tagsize <= 4)
        {
            if (!FileIO::Read(psi->pFileHandle, buf, tagsize))
            {
                SIMEXI_setlasterr("Couldn't read from file.");
                goto abort;
            }

            data = gettagdata(buf, tagsize);

            if (tag == 18)
            {
                if (!readtable(psi->pFileHandle, data, &pss->pvoltable))
                {
                    goto abort;
                }
            }
            else if (tag == 23)
            {
                if (!readtable(psi->pFileHandle, data, &pss->pbendtable))
                {
                    goto abort;
                }
            }
            else if (tag == 29)
            {
                if (!readtable(psi->pFileHandle, data, &pss->pvollfo))
                {
                    goto abort;
                }
            }
            else if (tag == 32)
            {
                if (!readtable(psi->pFileHandle, data, &pss->ppitchlfo))
                {
                    goto abort;
                }
            }
            else if (tag == 25)
            {
                envelopeoffset = data + FileIO::Tell(psi->pFileHandle) - 4;
            }
            else
            {
                sndtags[tag] = data;
            }
        }
        else
        {
            FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) + tagsize);
        }

    nexttag:
        if (!FileIO::Read(psi->pFileHandle, &tag, 1))
        {
            SIMEXI_setlasterr("Couldn't read from file.");
            goto abort;
        }
    }

    pss = psinfo->sound[psinfo->numsounds - 1];

    if (!SIMEXI_copytimbre(psi, pss, sndtags, envelopeoffset, platform, bigendian))
    {
        goto abort;
    }

    psinfo->cps = sndtags[0] / 16777216.0;
    psinfo->iscpsdefault = 0;
    psinfo->randdetune = sndtags[36];
    return 1;

abort:
    SIMEX_freesinfo(*ppsinfo);
    *ppsinfo = 0;
    return 0;
}
