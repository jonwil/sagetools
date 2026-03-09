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

int ImportOpenStub(SINSTANCE* pSInstance)
{
    return 1;
}

int ImportCloseStub(SINSTANCE* pSInstance)
{
    return 1;
}

int ExportOpenDefault(SINSTANCE* pSInstance)
{
    pSInstance->pFileHandle = FileIO::WOpen(pSInstance->fileName);

    if (!pSInstance->pFileHandle)
    {
        SIMEXI_setlasterr("Error opening file in SIMEX_open().");
        return 0;
    }

    return 1;
}

int ExportCloseDefault(SINSTANCE* pSInstance)
{
    int ret = 1;

    if (pSInstance->pFileHandle)
    {
        if (!FileIO::Close(pSInstance->pFileHandle))
        {
            SIMEXI_setlasterr("Error closing file in SIMEX_wclose().");
            ret = 0;
        }

        pSInstance->pFileHandle = 0;
    }

    return ret;
}

SFILEDRIVER sfiledriver[52] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {aboutwave, iswave, ImportOpenStub, infowave, readwave, ImportCloseStub, ExportOpenDefault, writewave, ExportCloseDefault, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {aboutmpeg, ismpeg, ImportOpenStub, infompeg, readmpeg, ImportCloseStub, 0, 0, 0, 1},
    {aboutsndstream, issndstream, ImportOpenStub, infosndstream, readsndstream, ImportCloseStub, ExportOpenDefault, writesndstream, ExportCloseDefault, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {AboutSndPlayer, ImportIsSndPlayer, ImportOpenStub, ImportInfoSndPlayer, ImportReadSndPlayer, ImportCloseStub, ExportOpenSndPlayer, ExportWriteSndPlayer, ExportCloseDefault, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
};

int SIMEX_about(int fileFormat, SABOUT* pSAbout)
{
    if (!pSAbout)
    {
        SIMEXI_setlasterr("A NULL SABOUT was passed to SIMEX_about.");
        return 0;
    }
    
    if (fileFormat < 0 || fileFormat >= 52)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_about is not supported.");
        return 0;
    }
    
    if (!sfiledriver[fileFormat].about)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_about is not supported.");
        return 0;
    }

    memset(pSAbout, 0, sizeof(SABOUT));
    return sfiledriver[fileFormat].about(pSAbout);
}

int SIMEX_id(const char* pFileName, __int64 fileOffset)
{
    int likelynum = -1;
    int likelypercent = 0;
    int percent = 0;
    FileHandle* pGStream = FileIO::Open(pFileName);

    if (pGStream)
    {
        FileIO::Seek(pGStream, fileOffset);
    }

    const int NUM_SLOWPARSERS = 1;
    int slowParsers[1] = { 34 };
    int i;

    for (i = 0; i < 52; i++)
    {
        int continueParsing = 1;

        for (int j = 0; j < NUM_SLOWPARSERS; j++)
        {
            if (i == slowParsers[j])
            {
                continueParsing = 0;
                break;
            }
        }

        if (continueParsing)
        {
            if (sfiledriver[i].is && sfiledriver[i].driverEnabled)
            {
                percent = sfiledriver[i].is(pFileName, fileOffset, pGStream);

                if (percent > likelypercent)
                {
                    likelypercent = percent;
                    likelynum = i;
                }

                if (pGStream)
                {
                    FileIO::Seek(pGStream, fileOffset);
                }
            }

            if (percent == 100)
            {
                break;
            }
        }
    }

    if (likelypercent != 100)
    {
        for (int j = 0; j < NUM_SLOWPARSERS; j++)
        {
            if (sfiledriver[slowParsers[j]].is && sfiledriver[slowParsers[j]].driverEnabled)
            {
                percent = sfiledriver[slowParsers[j]].is(pFileName, fileOffset, pGStream);

                if (percent > likelypercent)
                {
                    likelypercent = percent;
                    likelynum = slowParsers[j];
                }

                if (pGStream)
                {
                    FileIO::Seek(pGStream, fileOffset);
                }
            }

            if (percent == 100)
            {
                break;
            }
        }
    }

    if (pGStream)
    {
        FileIO::Close(pGStream);
    }

    return likelynum;
}

int SIMEX_open(const char* pFileName, __int64 fileOffset, int fileFormat, SINSTANCE** ppSInstance)
{
    int ret = 0;

    if (fileFormat < 0 || fileFormat >= 52 || !sfiledriver[fileFormat].open)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_open is not supported.");
        return 0;
    }

    *ppSInstance = (SINSTANCE*)Allocator::Alloc(sizeof(SINSTANCE));

    if (*ppSInstance)
    {
        memset(*ppSInstance, 0, sizeof(SINSTANCE));
        (*ppSInstance)->numelements = 1;
        (*ppSInstance)->fileOffset = fileOffset;
        (*ppSInstance)->fileformat = fileFormat;
        strcpy((*ppSInstance)->fileName, pFileName);
        (*ppSInstance)->pFileHandle = FileIO::Open(pFileName);

        if ((*ppSInstance)->pFileHandle)
        {
            FileIO::Seek((*ppSInstance)->pFileHandle, fileOffset);
        }

        ret = sfiledriver[fileFormat].open(*ppSInstance);
    }
    else
    {
        SIMEXI_setlasterr("Couldn't allocate memory for a SINSTANCE structure.");
    }

    if (ret <= 0)
    {
        if ((*ppSInstance)->pFileHandle)
        {
            FileIO::Close((*ppSInstance)->pFileHandle);
        }

        if (*ppSInstance)
        {
            Allocator::Free(*ppSInstance);
            *ppSInstance = 0;
        }
    }

    return ret;
}

int SIMEX_info(SINSTANCE* pSInstance, SINFO** ppSInfo, int element)
{
    if (pSInstance->fileformat < 0 || pSInstance->fileformat >= 52 || !sfiledriver[pSInstance->fileformat].info)
    {
        *ppSInfo = 0;
        SIMEXI_setlasterr("The file format passed to SIMEX_info is not supported.");
        return 0;
    }

    FileIO::Seek(pSInstance->pFileHandle, pSInstance->fileOffset);
    int ret = sfiledriver[pSInstance->fileformat].info(pSInstance, ppSInfo, element);
    return ret;
}

int SIMEX_read(SINSTANCE* pSInstance, SINFO* pSInfo, int element)
{
    if (!pSInfo)
    {
        SIMEXI_setlasterr("A NULL SINFO structure was passed to SIMEX_read.");
        return 0;
    }
    
    if (pSInstance->fileformat < 0 || pSInstance->fileformat >= 52 || !sfiledriver[pSInstance->fileformat].read)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_read is not supported.");
        return 0;
    }

    FileIO::Seek(pSInstance->pFileHandle, pSInstance->fileOffset);
    return sfiledriver[pSInstance->fileformat].read(pSInstance, pSInfo, element);
}

int SIMEX_close(SINSTANCE* pSInstance)
{
    int success = 0;

    if (pSInstance->fileformat < 0 || pSInstance->fileformat >= 52 || !sfiledriver[pSInstance->fileformat].close)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_close is not supported.");
        goto abort;
    }

    if (sfiledriver[pSInstance->fileformat].close(pSInstance) == 0)
    {
        goto abort;
    }

    success = 1;

abort:
    if (pSInstance->pFileHandle && !FileIO::Close(pSInstance->pFileHandle))
    {
        if (success)
        {
            SIMEXI_setlasterr("Problem closing FileHandle.");
        }

        success = 0;
    }

    if (pSInstance->pFileHandle2 && !FileIO::Close(pSInstance->pFileHandle2))
    {
        if (success)
        {
            SIMEXI_setlasterr("Problem closing FileHandle.");
        }

        success = 0;
    }

    if (!Allocator::Free(pSInstance))
    {
        if (success)
        {
            SIMEXI_setlasterr("Problem freeing SINSTANCE structure.");
        }

        success = 0;
    }

    return success;
}

int SIMEX_create(const char* pFileName, int fileFormat, SINSTANCE** ppSInstance)
{
    *ppSInstance = 0;

    if (fileFormat < 0 || fileFormat >= 52 || !sfiledriver[fileFormat].wopen)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_create is not supported.");
        goto abort;
    }

    *ppSInstance = (SINSTANCE*)Allocator::Alloc(sizeof(SINSTANCE));

    if (!*ppSInstance)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for a SINSTANCE structure.");
        goto abort;
    }

    memset(*ppSInstance, 0, sizeof(SINSTANCE));
    strcpy((*ppSInstance)->fileName, pFileName);
    (*ppSInstance)->fileformat = fileFormat;

    if (sfiledriver[fileFormat].wopen(*ppSInstance) <= 0)
    {
        goto abort;
    }

    return 1;

abort:
    if (*ppSInstance)
    {
        Allocator::Free(*ppSInstance);
    }

    *ppSInstance = 0;
    return 0;
}

int SIMEX_write(SINSTANCE* psi, SINFO* psinfo, int element)
{
    SINFO* ptestsinfo = 0;
    SINFO* ptempsinfo = 0;
    SSOUND* pss = 0;
    int ret = 1;
    SABOUT sAbout;
    int modes;

    if (!SIMEX_about(psi->fileformat, &sAbout))
    {
        ret = 0;
        goto abort;
    }

    modes = sAbout.playlocdefault + sAbout.playlocspu + sAbout.playlocmaincpu + sAbout.playlociopcpu + sAbout.playlocds2dhw + sAbout.playlocds3dhw + sAbout.playlocdsp + sAbout.playlocram + sAbout.playlocstream + sAbout.playlocgigasample;
    int i;
    int j;

    for (i = 0; i < psinfo->numsounds; i++)
    {
        pss = psinfo->sound[i];

        if (pss->samplerate <= 0)
        {
            SIMEXI_setlasterr("Invalid sample rate detected.");
            ret = 0;
            goto abort;
        }

        if (pss->numchannels <= 0)
        {
            SIMEXI_setlasterr("Invalid number of channels detected.");
            ret = 0;
            goto abort;
        }

        if (pss->length < 0)
        {
            SIMEXI_setlasterr("Invalid length detected.");
            ret = 0;
            goto abort;
        }

        if (pss->bitrate < 0)
        {
            SIMEXI_setlasterr("Invalid bit rate detected.");
            ret = 0;
            goto abort;
        }

        if (pss->numchannels > sAbout.maxchannels)
        {
            SIMEXI_setlasterr("This exporter is not capable of writing a file with this many channels in it.");
            ret = 0;
            goto abort;
        }

        if (!SIMEX_samplerepexpsupported(psi->fileformat, pss->samplerep))
        {
            SIMEXI_setlasterr("This exporter does not support the sample representation specified.");
            ret = 0;
            goto abort;
        }

        if (!SIMEXI_rendermodesupported(psi->fileformat, pss))
        {
            ret = 0;
            goto abort;
        }

        if (!modes)
        {
            pss->playloc = 0;
        }

        if (pss->samplerep == 14 && pss->samplerate != 32000 && pss->samplerate != 44100 && pss->samplerate != 48000)
        {
            SIMEXI_setlasterr("Samples compressed with MPEG Layers 1/2 must be 32000, 44100, or 48000 Hertz sources.");
            ret = 0;
            goto abort;
        }

        if (pss->samplerep == 15 && pss->samplerate != 16000 && pss->samplerate != 22050 && pss->samplerate != 24000 && pss->samplerate != 32000 && pss->samplerate != 44100 && pss->samplerate != 48000)
        {
            SIMEXI_setlasterr("Samples compressed with MPEG Layer 2 must be 16000, 22050, 24000, 32000, 44100, or 48000 Hertz sources.");
            ret = 0;
            goto abort;
        }

        if (pss->samplerep == 28)
        {
            SIMEXFILTERPARAM sfp;
            int samplerate = psinfo->sound[i]->samplerate;

            if (pss->samplerate > 48000)
            {
                ptempsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));

                if (!ptempsinfo)
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for SINFO structure.");
                    goto abort;
                }

                if (!SIMEX_copysinfo(ptempsinfo, psinfo))
                {
                    goto abort;
                }

                sfp.intval = 48000;
                SIMEX_filterssound(psinfo->sound[i], 120, &sfp);
                SIMEXI_warningcb("Out-of-range EA-XMA sample rate of %i detected.  Max is 48000. Resampling to %i...\n", samplerate, sfp.intval);
            }
        }
    
        if (pss->samplerep == 16 || pss->samplerep == 23 || pss->samplerep == 25 || pss->samplerep == 30 || pss->samplerep == 31 || pss->samplerep == 32)
        {
            if ((pss->samplerep == 23 || pss->samplerep == 25 || pss->samplerep == 30 || pss->samplerep == 31 || pss->samplerep == 32) && pss->tssidebandsize > 0)
            {
                SIMEXI_setlasterr("Time stretch cannot be applied to EALayer3 compressed samples.");
                ret = 0;
                goto abort;
            }

            SIMEXFILTERPARAM sfp;
            int samplerate = psinfo->sound[i]->samplerate;

            if ((pss->samplerep == 23 || pss->samplerep == 25 || pss->samplerep == 30 || pss->samplerep == 31 || pss->samplerep == 32) && pss->samplerate != 16000 && pss->samplerate != 22050 && pss->samplerate != 24000 && pss->samplerate != 32000 && pss->samplerate != 44100 && pss->samplerate != 48000)
            {
                ptempsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));

                if (!ptempsinfo)
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for SINFO structure.");
                    goto abort;
                }

                if (!SIMEX_copysinfo(ptempsinfo, psinfo))
                {
                    goto abort;
                }

                if (samplerate < 16000)
                {
                    sfp.intval = 16000;
                }
                else if (samplerate > 16000 && samplerate < 22050)
                {
                    sfp.intval = 22050;
                }
                else if (samplerate > 22050 && samplerate < 24000)
                {
                    sfp.intval = 24000;
                }
                else if (samplerate > 24000 && samplerate < 32000)
                {
                    sfp.intval = 32000;
                }
                else if (samplerate > 32000 && samplerate < 44100)
                {
                    sfp.intval = 44100;
                }
                else
                {
                    sfp.intval = 48000;
                }

                SIMEX_filterssound(psinfo->sound[i], 120, &sfp);
                SIMEXI_warningcb("Non-standard MPEG sample rate of %i detected.  Resampling to %i...\n", samplerate, sfp.intval);
            }

            if (pss->samplerep == 16 && pss->samplerate != 8000 && pss->samplerate != 11025 && pss->samplerate != 12000 && pss->samplerate != 16000 && pss->samplerate != 22050 && pss->samplerate != 24000 && pss->samplerate != 32000 && pss->samplerate != 44100 && pss->samplerate != 48000)
            {
                ptempsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));

                if (!ptempsinfo)
                {
                    SIMEXI_setlasterr("Couldn't allocate memory for SINFO structure.");
                    goto abort;
                }

                if (!SIMEX_copysinfo(ptempsinfo, psinfo))
                {
                    goto abort;
                }

                if (samplerate < 8000)
                {
                    sfp.intval = 8000;
                }
                else if (samplerate > 8000 && samplerate < 11025)
                {
                    sfp.intval = 11025;
                }
                else if (samplerate > 11025 && samplerate < 12000)
                {
                    sfp.intval = 12000;
                }
                else if (samplerate > 12000 && samplerate < 16000)
                {
                    sfp.intval = 16000;
                }
                else if (samplerate > 16000 && samplerate < 22050)
                {
                    sfp.intval = 22050;
                }
                else if (samplerate > 22050 && samplerate < 24000)
                {
                    sfp.intval = 24000;
                }
                else if (samplerate > 24000 && samplerate < 32000)
                {
                    sfp.intval = 32000;
                }
                else if (samplerate > 32000 && samplerate < 44100)
                {
                    sfp.intval = 44100;
                }
                else
                {
                    sfp.intval = 48000;
                }

                SIMEX_filterssound(psinfo->sound[i], 120, &sfp);
                SIMEXI_warningcb("Non-standard MPEG sample rate of %i detected.  Resampling to %i...\n", samplerate, sfp.intval);
            }
        }

        if (pss->sustainstart >= 0 && pss->sustainend < 0)
        {
            SIMEXI_setlasterr("Malformed loop points, only start point set.");
            ret = 0;
            goto abort;
        }

        if (pss->sustainend >= 0)
        {
            if (pss->sustainstart < 0)
            {
                SIMEXI_setlasterr("Malformed loop points, only end point set.");
                ret = 0;
                goto abort;
            }
            if (pss->sustainend >= pss->length)
            {
                SIMEXI_setlasterr("Malformed loop points, end point past sample data.");
                ret = 0;
                goto abort;
            }
        }

        if (pss->sustainstart >= 0 && pss->sustainend >= 0)
        {
            if (pss->sustainstart > pss->sustainend)
            {
                SIMEXI_setlasterr("Malformed loop points, start point located after end point.");
                ret = 0;
                goto abort;
            }
            if (pss->tssidebandsize > 0)
            {
                SIMEXI_setlasterr("Time stretch cannot be applied to samples that contain loop points.");
                ret = 0;
                goto abort;
            }
        }

        if (pss->tssidebandsize > 0 && pss->numchannels >= 2)
        {
            SIMEXI_setlasterr("Time stretch can only be applied to mono samples.");
            ret = 0;
            goto abort;
        }
    }

    if (psinfo->iscpsdefault && psi->fileformat == 35)
    {
        psinfo->cps = 5.0;
        psinfo->iscpsdefault = 0;
    }

    ptestsinfo = (SINFO*)Allocator::Alloc(sizeof(SINFO));

    if (!ptestsinfo)
    {
        SIMEXI_setlasterr("Couldn't allocate memory for testing SINFO.\n");
        goto abort;
    }

    MARKCHUNK* pmarkchunk;

    for (i = 0; i < pss->markerchunkcount; i++)
    {
        pmarkchunk = pss->pmarkerchunklist[i];

        for (j = 0; j < pmarkchunk->nummarkers; j++)
        {
            if (pmarkchunk->marks[j] && (pmarkchunk->marks[j]->position > pss->length || pmarkchunk->marks[j]->position < 0))
            {
                SIMEXI_setlasterr("Bad Marker detected.");
            }
        }
    }

    memcpy(ptestsinfo, psinfo, sizeof(SINFO));

    if (psi->fileformat < 0 || psi->fileformat >= 52 || !sfiledriver[psi->fileformat].write)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_write is not supported.");
        ret = 0;
        goto abort;
    }

    ret = sfiledriver[psi->fileformat].write(psi, psinfo, element);

    if (memcmp(ptestsinfo, psinfo, sizeof(SINFO)))
    {
        SIMEXI_setlasterr("INTERNAL ERROR: Source SINFO was modified by exporter.");
        ret = 0;
    }

    if (ptempsinfo)
    {
        SIMEX_copysinfo(psinfo, ptempsinfo);
        SIMEX_freesinfo(ptempsinfo);
    }

abort:
    if (ptestsinfo && !ptempsinfo)
    {
        Allocator::Free(ptestsinfo);
    }

    if (ptestsinfo && ptempsinfo)
    {
        SIMEX_freesinfo(ptestsinfo);
    }

    return ret;
}

int SIMEX_wclose(SINSTANCE* pSInstance)
{
    int ret;

    if (pSInstance->fileformat < 0 || pSInstance->fileformat >= 52 || !sfiledriver[pSInstance->fileformat].wclose)
    {
        SIMEXI_setlasterr("The file format passed to SIMEX_wclose is not supported.");
        return 0;
    }

    ret = sfiledriver[pSInstance->fileformat].wclose(pSInstance);

    if (!Allocator::Free(pSInstance))
    {
        if (ret == 1)
        {
            SIMEXI_setlasterr("Problem freeing SINSTANCE structure.");
        }

        ret = 0;
    }

    return ret;
}
