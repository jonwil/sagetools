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
#include <stdio.h>

int simexfiltersregistered;
int(*simexfilter[300])(SSOUND*, SIMEXFILTERPARAM*);
SIMEXFILTERABOUT simexfilterabout[300];
const char* simexfiltertypestrings[300];

int SIMEX_filtervbr(SSOUND* pss, SIMEXFILTERPARAM* sfp)
{
    pss->bitrate = sfp->intval;
    return 0;
}

void SIMEXI_filterregistervbr()
{
    static SIMEXFILTERPARAMDESC sfpd[1];
    SIMEXFILTERABOUT* psfa = &simexfilterabout[44];
    psfa->name = "Compression Quality";
    psfa->cmdline = "vbr";
    psfa->help = "The constant bit rate is used to set the level of compression. Note that VBR (variable bit rate) should be used instead when supported as it is superior in both interface usability and compression results.";
    psfa->numparams = 1;
    psfa->beforeImport = 0;
    psfa->afterImport = 1;
    psfa->params = sfpd;
    sfpd[0].name = "Variable Bit Rate Compression Quality";
    sfpd[0].cmdline = "quality";
    sfpd[0].help = "Specify compression quality, 0 = highest compression (bad sound quality), 100 = lowest compression (good sound quality)";
    sfpd[0].minval = 0.0;
    sfpd[0].maxval = 100.0;
    sfpd[0].valtype = 0;
    simexfilter[44] = SIMEX_filtervbr;
    simexfiltertypestrings[44] = "SIMEX_FILTER_VBR";
}

int SIMEX_filterplayloc(SSOUND* pss, SIMEXFILTERPARAM* sfp)
{
    char msgbuf[80];

    if (!strcmp(sfp->stringval, "spu"))
    {
        pss->playloc = 8;
    }
    else if (!strcmp(sfp->stringval, "maincpu"))
    {
        pss->playloc = 4;
    }
    else if (!strcmp(sfp->stringval, "iopcpu"))
    {
        pss->playloc = 256;
    }
    else if (!strcmp(sfp->stringval, "ds2dhw"))
    {
        pss->playloc = 1024;
    }
    else if (!strcmp(sfp->stringval, "ds3dhw"))
    {
        pss->playloc = 16;
    }
    else if (!strcmp(sfp->stringval, "dsp"))
    {
        pss->playloc = 512;
    }
    else if (!strcmp(sfp->stringval, "ram"))
    {
        pss->playloc = 2048;
    }
    else if (!strcmp(sfp->stringval, "stream"))
    {
        pss->playloc = 4096;
    }
    else if (!strcmp(sfp->stringval, "gigasample"))
    {
        pss->playloc = 0x2000;
    }
    else
    {
        if (strcmp(sfp->stringval, "default"))
        {
            sprintf(msgbuf, "Play-back location of %s is unknown.\n", sfp->stringval);
            SIMEXI_setlasterr(msgbuf);
            return -1;
        }

        pss->playloc = 0;
    }

    return 0;
}

void SIMEXI_filterregisterplayloc()
{
    static const char *stringlist[] = {"default", "spu", "maincpu", "iopcpu", "ds2dhw", "ds3dhw", "dsp", "ram", "stream", "gigasample"};
    static SIMEXFILTERPARAMDESC sfpd[1];
    SIMEXFILTERABOUT* psfa = &simexfilterabout[50];
    psfa->name = "Play-back Location";
    psfa->cmdline = "playloc";
    psfa->help = "The play-back location determines what system a sound will be played from.";
    psfa->numparams = 1;
    psfa->beforeImport = 0;
    psfa->afterImport = 1;
    psfa->params = sfpd;
    sfpd[0].name = "Play-back Location";
    sfpd[0].cmdline = "location";
    sfpd[0].help = "Location can be one of 'default', 'spu', 'maincpu', 'iopcpu', 'ds2dhw', 'ds3dhw', 'dsp', 'ram', 'stream', or 'gigasample'.";
    sfpd[0].valtype = 1;
    sfpd[0].stringlist = stringlist;
    simexfilter[50] = SIMEX_filterplayloc;
    simexfiltertypestrings[50] = "SIMEX_FILTER_PLAYLOC";
}

int SIMEX_filterembeduserdata(SSOUND* pss, SIMEXFILTERPARAM* sfp)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (!pss->puserdata[i])
        {
            pss->puserdata[i] = Allocator::Alloc(sfp->datasize);

            if (!pss->puserdata[i])
            {
                SIMEXI_setlasterr("Couldn't allocate mem to copy user data chunk.");
                return -1;
            }

            memcpy(pss->puserdata[i], sfp->pdata, sfp->datasize);
            pss->userdatasize[i] = sfp->datasize;
            return 0;
        }
    }

    SIMEXI_setlasterr("Out of list space for user data.");
    return -1;
}

void SIMEXI_filterregisterembeduserdata()
{
    static SIMEXFILTERPARAMDESC sfpd[1];
    SIMEXFILTERABOUT* psfa = &simexfilterabout[92];
    psfa->name = "Embed User Data";
    psfa->cmdline = "embeduser";
    psfa->help = "Attach user data to a sample.";
    psfa->numparams = 1;
    psfa->beforeImport = 0;
    psfa->afterImport = 1;
    psfa->params = sfpd;
    sfpd[0].name = "User Data";
    sfpd[0].cmdline = "data";
    sfpd[0].help = "Specify user data to attach to sample.";
    sfpd[0].minval = 0.0;
    sfpd[0].maxval = 0.0;
    sfpd[0].valtype = 2;
    simexfilter[92] = SIMEX_filterembeduserdata;
    simexfiltertypestrings[92] = "SIMEX_FILTER_EMBEDUSERDATA";
}

int SIMEX_filterclipsectionsound(SSOUND* psound, int startframe, int numframes)
{
    int j;
    MARKCHUNK* pmarkchunk;
    int srcframe;
    int dstframe;
    int copyframes;
    int framesfromhead;
    int i;
    short* ptemp;

    if (numframes <= 0)
    {
        return 0;
    }

    for (i = 0; i < psound->numchannels; i++)
    {
        ptemp = (short*)Allocator::Alloc(2 * numframes);

        if (!ptemp)
        {
            SIMEXI_setlasterr("Couldn't allocate memory to clip sample section.");
            return 0;
        }

        memset(ptemp, 0, 2 * numframes);

        if (startframe < 0)
        {
            srcframe = 0;
            dstframe = -startframe;
        }
        else
        {
            srcframe = startframe;
            dstframe = 0;
        }

        if (srcframe + numframes > psound->length)
        {
            copyframes = psound->length - srcframe;
        }
        else
        {
            copyframes = numframes;

            if (startframe < 0)
            {
                copyframes = copyframes + startframe;
            }
        }

        if (dstframe + copyframes > numframes)
        {
            copyframes -= dstframe + copyframes - numframes;
        }

        if (psound->track[i] && copyframes > 0)
        {
            memcpy(&ptemp[dstframe], &psound->track[i][srcframe], 2 * copyframes);
        }

        if (psound->track[i])
        {
            Allocator::Free(psound->track[i]);
        }

        psound->track[i] = ptemp;
    }

    psound->length = numframes;

    if (psound->sustainend > 0)
    {
        psound->sustainstart -= startframe;
        psound->sustainend -= startframe;

        if (psound->sustainend >= psound->length || psound->sustainstart < 0)
        {
            psound->sustainstart = -1;
            psound->sustainend = -1;
        }
    }

    framesfromhead = numframes + startframe;

    if (framesfromhead <= 0)
    {
        for (i = 0; i < psound->markerchunkcount; i++)
        {
            pmarkchunk = psound->pmarkerchunklist[i];

            for (j = 0; j < pmarkchunk->nummarkers; j++)
            {
                if (pmarkchunk->marks[j])
                {
                    if (pmarkchunk->marks[j]->length > 0)
                    {
                        Allocator::Free(pmarkchunk->marks[j]->string);
                    }

                    Allocator::Free(pmarkchunk->marks[j]);
                    pmarkchunk->marks[j] = 0;
                }
            }

            Allocator::Free(psound->pmarkerchunklist[i]);
        }
        psound->markerchunkcount = 0;
    }

    for (i = 0; i < psound->markerchunkcount; i++)
    {
        pmarkchunk = psound->pmarkerchunklist[i];

        for (j = 0; j < pmarkchunk->nummarkers; j++)
        {
            if (pmarkchunk->marks[j])
            {
                if (startframe >= 0)
                {
                    if (pmarkchunk->marks[j]->position >= startframe && pmarkchunk->marks[j]->position <= startframe + numframes)
                    {
                        pmarkchunk->marks[j]->position -= startframe;
                    }
                    else
                    {
                        if (pmarkchunk->marks[j]->length > 0)
                        {
                            Allocator::Free(pmarkchunk->marks[j]->string);
                        }

                        Allocator::Free(pmarkchunk->marks[j]);
                        pmarkchunk->marks[j] = 0;
                    }
                }
                else if (pmarkchunk->marks[j]->position >= framesfromhead)
                {
                    if (pmarkchunk->marks[j]->length > 0)
                    {
                        Allocator::Free(pmarkchunk->marks[j]->string);
                    }

                    Allocator::Free(pmarkchunk->marks[j]);
                    pmarkchunk->marks[j] = 0;
                }
                else
                {
                    pmarkchunk->marks[j]->position -= startframe;
                }
            }
        }
    }

    return 1;
}

int SIMEX_filtercrop(SSOUND* pss, SIMEXFILTERPARAM* sfp)
{
    if (SIMEX_filterclipsectionsound(pss, sfp->intval, sfp[1].intval))
    {
        return 1;
    }

    return -1;
}

void SIMEXI_filterregistercrop()
{
    static SIMEXFILTERPARAMDESC sfpd[2];
    SIMEXFILTERABOUT* psfa = &simexfilterabout[150];
    psfa->name = "Crop";
    psfa->cmdline = "crop";
    psfa->help = "Crop a section of a sample.";
    psfa->numparams = 2;
    psfa->beforeImport = 0;
    psfa->afterImport = 1;
    psfa->params = sfpd;
    sfpd[0].name = "Start Frame";
    sfpd[0].cmdline = "startframe";
    sfpd[0].help = "Specify frame to begin crop at. Negative values insert silence at start of crop.";
    sfpd[0].minval = -2147483647.0;
    sfpd[0].maxval = 2147483647.0;
    sfpd[0].valtype = 0;
    sfpd[1].name = "Frames";
    sfpd[1].cmdline = "frames";
    sfpd[1].help = "Specify number of frames to crop. Specifying past original sample will insert silence at end of crop.";
    sfpd[1].minval = 1.0;
    sfpd[1].maxval = 2147483647.0;
    sfpd[1].valtype = 0;
    simexfilter[150] = SIMEX_filtercrop;
    simexfiltertypestrings[150] = "SIMEX_FILTER_CROP";
}

void SIMEXI_registerfilters()
{
    SIMEXI_filterregistervbr();
    SIMEXI_filterregisterplayloc();
    SIMEXI_filterregisterembeduserdata();
    SIMEXI_filterregisterresample();
    SIMEXI_filterregistercrop();
}

SIMEXFILTERABOUT* SIMEX_filterabout(int filtertype)
{
    if (!simexfiltersregistered)
    {
        SIMEXI_registerfilters();
        simexfiltersregistered = 1;
    }

    if (filtertype < 0 || filtertype >= 300)
    {
        SIMEXI_setlasterr("Filter type is out of range.");
        return 0;
    }

    if (simexfilterabout[filtertype].name)
    {
        return &simexfilterabout[filtertype];
    }

    return 0;
}

int SIMEX_filterssound(SSOUND* pss, int filtertype, SIMEXFILTERPARAM* psfp)
{
    if (!simexfiltersregistered)
    {
        SIMEXI_registerfilters();
        simexfiltersregistered = 1;
    }

    if (simexfilter[filtertype])
    {
        return simexfilter[filtertype](pss, psfp);
    }

    SIMEXI_setlasterr("Filter type is not implemented.");
    return -1;
}
