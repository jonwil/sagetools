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
#include "cmn\isimex.h"
#include "cmn\fileio.h"

int aboutsndstream(SABOUT* pSAbout)
{
    pSAbout->imp.commonvers[0] = -1;
    pSAbout->imp.platformvers[0] = 1;
    pSAbout->imp.platformvers[1] = 2;
    pSAbout->imp.platformvers[2] = -1;
    pSAbout->imp.samplereps[0] = 10;
    pSAbout->imp.samplereps[1] = -1;
    pSAbout->exp.commonvers[0] = -1;
    pSAbout->exp.platformvers[0] = 2;
    pSAbout->exp.platformvers[1] = -1;
    pSAbout->exp.samplereps[0] = 10;
    pSAbout->exp.samplereps[1] = -1;

    pSAbout->maxelements = 1;
    pSAbout->maxtimbres = 1;
    pSAbout->maxchannels = 6;
    pSAbout->canimport = 1;
    pSAbout->canexport = 1;
    pSAbout->cbr = 1;
    pSAbout->vbr = 1;
    pSAbout->cps = 1;
    strcpy(pSAbout->formatword, "sndstream");
    strcpy(pSAbout->formatname, "SND Generic stream");
    return 1;
}

int issndstream(const char* pFileName, __int64 fileOffset, FileHandle* pgs)
{
    return isstream(pgs, 8, 0);
}

int infosndstream(SINSTANCE* psi, SINFO** ppsinfo, int element)
{
    if (element)
    {
        SIMEXI_setlasterr("An attempt to get info from an element other than 0 was made.");
        return 0;
    }

    return infostream(psi, ppsinfo, 8, 0);
}

int readsndstream(SINSTANCE* psi, SINFO* psinfo, int element)
{
    element = element;
    return readstream(psi, psinfo, 8, 38, 1);
}

int writesndstreamtimbre(FileHandle* pgs, SINFO* psinfo)
{
    SSOUND* pss = psinfo->sound[0];
    int numframes = pss->length;
    puttagmv(pgs, 0x80, 2, 3);
    puttagmv(pgs, 0x85, 0, numframes);
    puttagmv(pgs, 0x82, 1, pss->numchannels);
    puttagmv(pgs, 0xA0, 10, pss->samplerep);
    puttagmv(pgs, 0x84, 48000, pss->samplerate);
    return 1;
}

int writesndstream(SINSTANCE* psi, SINFO* psinfo, int element)
{
    if (element)
    {
        SIMEXI_setlasterr("SND stream files can only contain element 0.");
        return 0;
    }

    SSOUND* pss = psinfo->sound[0];
    pss->playloc = 4;

    if (pss->samplerep != 4 && pss->samplerep != 22 && pss->samplerep != 10 && pss->samplerep != 23)
    {
        SIMEXI_setlasterr("Cross-platform SND stream files can only use EAXA, MicroTalk or MPEG EALAYER3 samplerep's.");
        return 0;
    }

    return writestream(psi, psinfo, 1, 8, 102);
}
