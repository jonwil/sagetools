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
#include "cmn\sstrmutil.h"
#include "cmn\sbnkutil.h"
#include "cmn\isimex.h"
#include "cmn\fileio.h"

struct SCHUNKHDR
{
    int type;
    int size;
};

struct GENTAGGEDPATCH
{
    unsigned char id[4];
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    unsigned char PatchVersion;
    unsigned char pad[1];
};

struct TAGGEDPATCH
{
    short id;
    unsigned char platform;
    unsigned char flags;
    int hdrsize;
};

__int64 getstreampatchoffset(FileHandle* pgs, int bigendian)
{
    __int64 filesize;
    bool trueCondition;
    SCHUNKHDR chunkhdr;
    __int64 offset;
    __int64 lastfilepos;

    lastfilepos = 0;
    filesize = FileIO::Length(pgs);
    trueCondition = true;

    while (trueCondition)
    {
        if (FileIO::Tell(pgs) + 8 > filesize)
        {
            return 0;
        }

        if (!FileIO::Read(pgs, &chunkhdr, 8))
        {
            return 0;
        }

        if (GetM(&chunkhdr, 4) == 'SCHl')
        {
            return FileIO::Tell(pgs);
        }

        offset = GetI(&chunkhdr.size, 4);
        offset = FileIO::Tell(pgs) + offset - 8;

        if (!FileIO::Seek(pgs, offset))
        {
            return 0;
        }

        if (FileIO::Tell(pgs) <= lastfilepos)
        {
            return 0;
        }
        else
        {
            lastfilepos = FileIO::Tell(pgs);
        }
    }

    return 0;
}

int isstream(FileHandle* pgs, int platform, int bigendian)
{
    GENTAGGEDPATCH genpatch;
    __int64 offset;
    TAGGEDPATCH patch;

    if (!pgs)
    {
        return 0;
    }

    if (FileIO::Length(pgs) < 50)
    {
        return 0;
    }

    offset = getstreampatchoffset(pgs, bigendian);

    if (!offset)
    {
        return 0;
    }

    FileIO::Seek(pgs, offset);

    if (platform != 8)
    {
        if (!FileIO::Read(pgs, &patch, 8))
        {
            return 0;
        }

        if (GetM(&patch, 2) == 20564 && patch.platform == platform)
        {
            return 100;
        }
    }
    else
    {
        if (!FileIO::Read(pgs, &genpatch, 8))
        {
            return 0;
        }

        if (genpatch.id[0] == 71 && genpatch.id[1] == 83 && genpatch.id[2] == 84 && genpatch.id[3] == 82)
        {
            return 100;
        }
    }

    return 0;
}

int infostream(SINSTANCE* psi, SINFO** ppsinfo, int platform, int bigendian)
{
    __int64 patchoffset;
    *ppsinfo = 0;
    patchoffset = getstreampatchoffset(psi->pFileHandle, bigendian);

    if (!patchoffset)
    {
        return 1;
    }

    return iinfopatch(psi, ppsinfo, platform, bigendian, (int)patchoffset);
}

int readstream(SINSTANCE* psi, SINFO* psinfo, int platform, int format, int bigendian)
{
    bool trueCondition;
    SCHUNKHDR chunkhdr;
    SSOUND* psound;
    SCOMPSTATE* pcompstate[6] = { 0 };
    __int64 sampleoffsets[6] = { 0 };
    __int64 oldpos;
    int framesread = 0;
    int numframes;
    int val;
    int chunksize;
    int i;
    int ret = 0;

    psound = psinfo->sound[0];

    if (SIMEXI_allocatecompstate(pcompstate, 6) < 0)
    {
        goto abort;
    }

    if (SIMEXI_allocatetracks(psound) < 0)
    {
        goto abort;
    }

    for (i = 0; i < 6; i++)
    {
        pcompstate[i]->isstream = 1;
    }

    trueCondition = true;

    while (trueCondition)
    {
        oldpos = FileIO::Tell(psi->pFileHandle);
        FileIO::Read(psi->pFileHandle, &chunkhdr, 8);
        chunksize = GetI(&chunkhdr.size, 4);

        if (GetM(&chunkhdr, 4) == 'SCDl')
        {
            FileIO::Read(psi->pFileHandle, &numframes, 4);

            if (psound->platformver >= 1 || platform == 10 || platform == 14 || platform == 15 || platform == 13)
            {
                for (i = 0; i < psound->numchannels; i++)
                {
                    FileIO::Read(psi->pFileHandle, &val, 4);

                    if (bigendian)
                    {
                        sampleoffsets[i] = GetM(&val, 4);
                    }
                    else
                    {
                        sampleoffsets[i] = GetI(&val, 4);
                    }
                }
            }

            if (bigendian)
            {
                numframes = GetM(&numframes, 4);
            }
            else
            {
                numframes = GetI(&numframes, 4);
            }

            if (psound->samplerep == 10)
            {
                if (!psound->platformver && platform != 10)
                {
                    FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) + 8);
                }
                else
                {
                    for (i = 0; i < psound->numchannels; i++)
                    {
                        sampleoffsets[i] += 4;
                    }
                }
            }
            else if (psound->samplerep == 4 || psound->samplerep == 22)
            {
                FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) + 1);
            }

            if (psound->samplerep == 15)
            {
                pcompstate[0]->mp3.chunksize = chunksize;

                if (!framesread && !(numframes % 1152))
                {
                    numframes -= 481;
                }
            }

            if (psound->samplerep == 16)
            {
                pcompstate[0]->mp3.chunksize = chunksize;
            }

            if (psound->samplerep == 23)
            {
                pcompstate[0]->chunksize = chunksize;
            }

            if (psound->platformver >= 1 || platform == 10 || platform == 14 || platform == 15 || platform == 13)
            {
                for (i = 0; i < psound->numchannels; i++)
                {
                    sampleoffsets[i] += (int)FileIO::Tell(psi->pFileHandle);
                }
            }
            else
            {
                sampleoffsets[0] = (int)FileIO::Tell(psi->pFileHandle);
            }

            psound->readinfo.codecversion = PlatformToCodecVer(platform, psound->platformver, psound->samplerep);
            int originalSampleRep = psound->samplerep;

            if (psi->fileformat == 43 || psi->fileformat == 45 || psi->fileformat == 51 || psi->fileformat == 47)
            {
                psound->samplerep = 7;
            }

            if (!SIMEXI_importsamples(psi->pFileHandle, sampleoffsets, 0, framesread, numframes, psound->numchannels, psound->samplerep, 0, pcompstate, psound->track, psound))
            {
                goto abort;
            }

            psound->samplerep = originalSampleRep;
            framesread += numframes;
        }
        else if (GetM(&chunkhdr, 4) == 'SCEl')
        {
            ret = 1;
            goto abort;
        }

        FileIO::Seek(psi->pFileHandle, oldpos + chunksize);
    }

abort:
    SIMEXI_freecompstate(pcompstate, 6);
    return ret;
}

int writeheaderchunk(FileHandle* pgs, SINFO* psinfo, int bigendian, int platform, int flags)
{
    GENTAGGEDPATCH genpatch;
    __int64 chunkend;
    SCHUNKHDR chunkhdr;
    SSOUND* psound;
    int dummy;
    __int64 chunkstart;
    int size;
    unsigned int cpsinteger;
    __int64 align;
    TAGGEDPATCH patch;
    int i;

    dummy = 0;
    size = 0;
    psound = psinfo->sound[0];
    chunkstart = FileIO::Tell(pgs);
    PutM(&chunkhdr.type, 'SCHl', 4);
    PutI(&chunkhdr.size, size, 4);
    FileIO::Write(pgs, &chunkhdr, 8);

    if (platform != 8)
    {
        PutM(&patch, 'PT', 2);
        patch.platform = platform;
        patch.flags = 0;
        FileIO::Write(pgs, &patch, 4);
    }
    else
    {
        PutM(&genpatch, 'GSTR', 4);
        genpatch.MajorVersion = 1;
        genpatch.MinorVersion = 0;
        genpatch.PatchVersion = 0;
        FileIO::Write(pgs, &genpatch, 8);
    }

    cpsinteger = psinfo->cps * 16777216.0;

    if (platform != 8)
    {
        puttagmv(pgs, 0, 251658240, cpsinteger);
    }
    else
    {
        puttagmv(pgs, 0, 83886080, cpsinteger);
    }

    for (i = 0; i < 4; i++)
    {
        if (psound->puserdata[i])
        {
            SIMEXI_aligntag(pgs, psound->userdatasize[i], 4);
            puttagdata(pgs, 0x14u, psound->puserdata[i], psound->userdatasize[i]);
        }
    }

    puttagmv(pgs, 5, 50, psound->bitrate);
    puttagmv(pgs, 6, 0, 101);
    putmarker(pgs, 253);

    if (platform == 8)
    {
        writesndstreamtimbre(pgs, psinfo);
    }
    else
    {
        SIMEXI_setlasterr("Unsupported stream platform in writeheaderchunk.");
        return 0;
    }

    putmarker(pgs, 0xFFu);
    align = FileIO::Tell(pgs) % 4;

    if (align)
    {
        FileIO::Write(pgs, &dummy, 4 - align);
    }

    chunkend = FileIO::Tell(pgs);
    FileIO::Seek(pgs, chunkstart);
    PutM(&chunkhdr.type, 'SCHl', 4);
    PutI(&chunkhdr.size, chunkend - chunkstart, 4);
    FileIO::Write(pgs, &chunkhdr, 8);
    FileIO::Seek(pgs, chunkend);
    return 1;
}

__int64 writenumchunkschunk(FileHandle* pgs, int bigendian)
{
    SCHUNKHDR chunkhdr;
    int tempnumchunks;
    __int64 position;

    PutM(&chunkhdr.type, 'SCCl', 4);
    PutI(&chunkhdr.size, 12u, 4);
    FileIO::Write(pgs, &chunkhdr, 8);
    position = FileIO::Tell(pgs);
    tempnumchunks = 0;
    FileIO::Write(pgs, &tempnumchunks, 4);
    return position;
}

int SIMEXI_writedatachunk(SINSTANCE* psi, FileHandle* pgs, SINFO* psinfo, int curframe, int frames, SCOMPSTATE* pcompstate[], int bigendian, int flags)
{
    int framesinchunk;
    SCHUNKHDR chunkhdr;
    SSOUND* psound;
    __int64 trackoffsetsoffset;
    __int64 endchunkoffset;
    int dummy;
    int paddedframes;
    __int64 startchunkoffset;
    int trackoffsets[6];
    int i;

    dummy = 0;
    startchunkoffset = FileIO::Tell(pgs);
    PutM(&chunkhdr, 'SCDl', 4);
    chunkhdr.size = 0;
    FileIO::Write(pgs, &chunkhdr, 8);
    psound = psinfo->sound[0];

    if (curframe + frames > psound->length)
    {
        frames = psound->length - curframe;
    }

    paddedframes = frames;

    if (paddedframes > 0)
    {
        if (!curframe)
        {
            if (psound->samplerep == 15)
            {
                paddedframes -= 481;
            }

            if (psound->samplerep == 16 || psound->samplerep == 23)
            {
                paddedframes -= 1105;
            }
        }
        if (curframe + frames >= psound->length)
        {
            if (psound->samplerep == 15)
            {
                paddedframes += 481;
            }

            if (psound->samplerep == 16 || psound->samplerep == 23)
            {
                paddedframes += 1105;
            }
        }
    }

    if (bigendian)
    {
        PutM(&framesinchunk, paddedframes, 4);
    }
    else
    {
        PutI(&framesinchunk, paddedframes, 4);
    }

    FileIO::Write(pgs, &framesinchunk, 4);
    trackoffsetsoffset = FileIO::Tell(pgs);

    for (i = 0; i < psound->numchannels; i++)
    {
        FileIO::Write(pgs, &trackoffsets[i], 4);
    }

    if (!SIMEXI_exportsamplesfile(psi, pgs, curframe, frames, flags, pcompstate, trackoffsets, psound))
    {
        return 0;
    }

    endchunkoffset = FileIO::Tell(pgs);

    while (endchunkoffset % 4)
    {
        dummy = 0;
        FileIO::Write(pgs, &dummy, 1);
        endchunkoffset++;
    }

    FileIO::Seek(pgs, startchunkoffset);
    PutM(&chunkhdr, 'SCDl', 4);
    PutI(&chunkhdr.size, endchunkoffset - startchunkoffset, 4);
    FileIO::Write(pgs, &chunkhdr, 8);
    FileIO::Seek(pgs, trackoffsetsoffset);

    for (i = 0; i < psound->numchannels; i++)
    {
        if (bigendian)
        {
            PutM(&trackoffsets[i], trackoffsets[i], 4);
        }
        else
        {
            PutI(&trackoffsets[i], trackoffsets[i], 4);
        }

        FileIO::Write(pgs, &trackoffsets[i], 4);
    }

    FileIO::Seek(pgs, endchunkoffset);
    return 1;
}

int writeendchunk(FileHandle* pgs, int bigendian)
{
    __int64 chunkend;
    SCHUNKHDR chunkhdr;
    __int64 chunkstart;

    chunkstart = FileIO::Tell(pgs);
    PutM(&chunkhdr.type, 'SCEl', 4);
    PutI(&chunkhdr.size, 8u, 4);
    FileIO::Write(pgs, &chunkhdr, 8);
    chunkend = FileIO::Tell(pgs);
    return 1;
}

int writestream(SINSTANCE* psi, SINFO* psinfo, int bigendian, int platform, int flags)
{
    int curframe;
    SIMEXFILTERPARAM cropsfp[2];
    int framesperchunk;
    SSOUND* psound;
    int ret;
    unsigned int numberofdatachunks;
    int frameswritten;
    SCOMPSTATE* pcompstate[6] = { 0 };
    int frames;
    int tempnumchunks;
    int minblockframes;
    int i;
    __int64 numchunkspos;

    numberofdatachunks = 0;
    frameswritten = 0;
    minblockframes = 1;
    ret = 0;

    if (SIMEXI_allocatecompstate(pcompstate, 6) < 0)
    {
        goto abort;
    }

    psound = psinfo->sound[0];

    if (psound->sustainend > 0)
    {
        SIMEXI_setlasterr("SND streams cannot contain loop points.");
        goto abort;
    }

    if (psound->samplerep == 10)
    {
        minblockframes = 28;
    }
    else if (psound->samplerep == 20)
    {
        minblockframes = 64;
    }
    else if (psound->samplerep == 4 || psound->samplerep == 22)
    {
        minblockframes = 432;
    }
    else if (psound->samplerep == 14)
    {
        minblockframes = 384;
    }
    else if (psound->samplerep == 15)
    {
        minblockframes = 1152;
    }
    else if (psound->samplerep == 16 || psound->samplerep == 23)
    {
        if (psound->samplerate >= 32000)
        {
            minblockframes = 1152;
        }
        else
        {
            minblockframes = 576;
        }
    }

    if (platform == 6 && psound->samplerep == 7)
    {
        minblockframes = 16;
    }

    if (psound->samplerep == 20)
    {
        cropsfp[0].intval = 0;
        cropsfp[1].intval = minblockframes * ((psound->length + minblockframes - 1) / minblockframes);
        SIMEX_filterssound(psound, 150, cropsfp);
    }

    if (psinfo->cps <= 0.0)
    {
        psinfo->cps = 15.0;
    }

    framesperchunk = psound->samplerate / psinfo->cps;

    if (framesperchunk % minblockframes)
    {
        framesperchunk += minblockframes - framesperchunk % minblockframes;
    }

    for (i = 0; i < 6; i++)
    {
        pcompstate[i]->isstream = 1;
    }

    curframe = 0;

    if (writeheaderchunk(psi->pFileHandle, psinfo, bigendian, platform, flags) == 0)
    {
        goto abort;
    }

    numchunkspos = writenumchunkschunk(psi->pFileHandle, bigendian);

    while (curframe < psound->length)
    {
        framesperchunk = (int)((float)(numberofdatachunks + 1) * psound->samplerate / psinfo->cps) - frameswritten;
        frames = framesperchunk;

        if (frames % minblockframes)
        {
            frames += minblockframes - frames % minblockframes;
        }

        frameswritten += frames;

        if (!SIMEXI_writedatachunk(psi, psi->pFileHandle, psinfo, curframe, frames, pcompstate, bigendian, flags))
        {
            goto abort;
        }

        numberofdatachunks++;
        curframe += frames;
    }

    writeendchunk(psi->pFileHandle, bigendian);
    FileIO::Seek(psi->pFileHandle, numchunkspos);

    if (bigendian)
    {
        PutM(&tempnumchunks, numberofdatachunks, 4);
    }
    else
    {
        PutI(&tempnumchunks, numberofdatachunks, 4);
    }

    FileIO::Write(psi->pFileHandle, &tempnumchunks, 4);
    ret = 1;

abort:
    SIMEXI_freecompstate(pcompstate, 6);
    return ret;
}
