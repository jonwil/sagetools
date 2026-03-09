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

double mpegsamplerate[2][4] = { {22.05, 24.0, 16.0, 0.0}, {44.1, 48.0, 32.0, 0.0} };

int mpegbitrate[2][3][15] =
{
    {
        {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
        {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}
    },

    {
        {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
        {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
        {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
    }
};

int SIMEXI_mpegparseheader(unsigned int hdr, MPEGAUDIOHDR* pmah)
{
    if ((hdr & 0xFFE00000) != 0xFFE00000)
    {
        return 0;
    }

    pmah->layer = 4 - ((hdr >> 17) & 3);
    pmah->crc = ((hdr >> 16) & 1) == 0;
    pmah->bitrateindex = (hdr >> 12) & 0xF;
    pmah->padded = ((hdr >> 9) & 1);
    pmah->mode = (hdr >> 6) & 3;
    pmah->modeext = (hdr >> 4) & 3;

    if (pmah->layer == 4)
    {
        return 0;
    }

    if (pmah->bitrateindex == 15)
    {
        return 0;
    }
    
    pmah->version = (hdr >> 19) & 3;

    if (pmah->version == 1)
    {
        return 0;
    }

    pmah->samplerateindex = (hdr >> 10) & 3;

    if (pmah->samplerateindex == 3)
    {
        return 0;
    }

    pmah->channels = (pmah->mode != 3) + 1;
    pmah->samplerate = mpegsamplerate[1][pmah->samplerateindex] * 1000.0;

    if (pmah->version == 2)
    {
        pmah->samplerate >>= 1;
    }

    if (!pmah->version)
    {
        pmah->samplerate >>= 2;
    }

    if (!pmah->bitrateindex)
    {
        return 0;
    }
    
    if (pmah->version == 3)
    {
        pmah->bitrate = mpegbitrate[1][pmah->layer - 1][pmah->bitrateindex];
    }
    else
    {
        pmah->bitrate = mpegbitrate[0][pmah->layer - 1][pmah->bitrateindex];
    }

    if (pmah->layer == 1)
    {
        pmah->framebytes = 12000 * pmah->bitrate / pmah->samplerate;
        pmah->framebytes = 4 * (pmah->framebytes + pmah->padded);
        pmah->numframes = 384;
    }
    else
    {
        pmah->numframes = 1152;
        pmah->framebytes = 144000 * pmah->bitrate / pmah->samplerate;
        if (pmah->layer == 3 && pmah->version != 3)
        {
            pmah->framebytes >>= 1;
            pmah->numframes = 576;
        }

        if (pmah->padded)
        {
            pmah->framebytes++;
        }
    }

    pmah->framebytes -= 4;
    return pmah->framebytes;
}
