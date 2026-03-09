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

__int64 SIMEXI_locatempegsync(FileHandle* pgs)
{
    int ismpeg;
    unsigned int hdr;
    MPEGAUDIOHDR mah;
    int done;
    __int64 initialoffset;
    __int64 location;
    int framebytes;
    int i;

    initialoffset = FileIO::Tell(pgs);
    location = 0;
    done = 0;

    do
    {
        FileIO::Seek(pgs, location);
        ismpeg = 1;

        for (i = 0; i < 7; i++)
        {
            if (FileIO::Read(pgs, &hdr, 4) == 4)
            {

                framebytes = SIMEXI_mpegparseheader(GetM(&hdr, 4), &mah);

                if (!framebytes)
                {
                    ismpeg = 0;
                    break;
                }

                FileIO::Seek(pgs, FileIO::Tell(pgs) + framebytes);
            }
            else
            {
                done = 1;
                ismpeg = 0;
                break;
            }
        }

        if (ismpeg)
        {
            FileIO::Seek(pgs, initialoffset);
            return location;
        }

        location++;
    } while (!done);

    FileIO::Seek(pgs, initialoffset);
    return -1;
}

int aboutmpeg(SABOUT* pSAbout)
{
    pSAbout->imp.commonvers[0] = -1;
    pSAbout->imp.platformvers[0] = -1;
    pSAbout->imp.samplereps[0] = 16;
    pSAbout->imp.samplereps[1] = -1;
    pSAbout->exp.commonvers[0] = -1;
    pSAbout->exp.platformvers[0] = -1;
    pSAbout->exp.samplereps[0] = -1;

    pSAbout->maxelements = 1;
    pSAbout->maxtimbres = 1;
    pSAbout->maxchannels = 2;

    pSAbout->canimport = 1;
    pSAbout->canexport = 0;
    
    pSAbout->cbr = 1;
    pSAbout->vbr = 1;
    strcpy(pSAbout->formatword, "mpeg");
    strcpy(pSAbout->formatname, "MPEG");
    return 1;
}

int ismpeg(const char* pFileName, __int64 fileOffset, FileHandle* pgs)
{
    if (!pgs)
    {
        return 0;
    }

    if (SIMEXI_locatempegsync(pgs) >= 0)
    {
        return 75;
    }

    return 0;
}

int infompeg(SINSTANCE* psi, SINFO** ppsinfo, int element)
{
    __int64 filesize;
    unsigned int hdr;
    MPEGAUDIOHDR mah = { 0 };
    SSOUND* psound;
    int done;
    SINFO* psinfo;
    int framebytes;

    psinfo = 0;
    filesize = FileIO::Length(psi->pFileHandle);
    *ppsinfo = 0;

    if (element)
    {
        SIMEXI_setlasterr("An attempt to get info from an element other than 0 was made.");
        return 0;
    }

    FileIO::Seek(psi->pFileHandle, SIMEXI_locatempegsync(psi->pFileHandle));
    *ppsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));
    psinfo = *ppsinfo;

    if (!psinfo)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for an SINFO structure.");
        return 0;
    }

    SIMEX_defaultsinfo(psinfo);
    psound = psinfo->sound[0];
    psound->length = 0;
    done = 0;

    do
    {
        if (FileIO::Tell(psi->pFileHandle) + 4 > filesize)
        {
            done = 1;
        }
        else if (FileIO::Read(psi->pFileHandle, &hdr, 4) == 4)
        {
            framebytes = SIMEXI_mpegparseheader(GetM(&hdr, 4), &mah);

            if (framebytes)
            {
                FileIO::Seek(psi->pFileHandle, FileIO::Tell(psi->pFileHandle) + framebytes);
                psound->length += mah.numframes;
            }
            else
            {
                done = 1;
            }
        }
        else
        {
            done = 1;
        }
    } while (!done);
    
    psound->numchannels = mah.channels;
    psound->samplerate = mah.samplerate;
    psound->bitrate = 1000 * mah.bitrate;

    if (mah.layer == 3)
    {
        psound->samplerep = 16;
    }
    else if (mah.layer == 2)
    {
        psound->samplerep = 15;
    }
    else if (mah.layer == 1)
    {
        psound->samplerep = 14;
    }

    return 1;
}

int readmpeg(SINSTANCE* psi, SINFO* psinfo, int element)
{
    SSOUND* psound;
    SCOMPSTATE* pcompstate[6] = { 0 };
    __int64 sampleoffsets[6] = { 0 };
    int ret;

    ret = 0;
    element = element;
    psound = psinfo->sound[0];

    if (SIMEXI_allocatecompstate(pcompstate, 6) < 0)
    {
        goto abort;
    }

    if (SIMEXI_allocatetracks(psound) < 0)
    {
        goto abort;
    }

    FileIO::Seek(psi->pFileHandle, SIMEXI_locatempegsync(psi->pFileHandle));
    sampleoffsets[0] = FileIO::Tell(psi->pFileHandle);
    ret = SIMEXI_importsamples(psi->pFileHandle, sampleoffsets, 0, 0, psound->length, psound->numchannels, psound->samplerep, 0, pcompstate, psound->track, psound);

abort:
    SIMEXI_freecompstate(pcompstate, 6);
    return ret;
}
