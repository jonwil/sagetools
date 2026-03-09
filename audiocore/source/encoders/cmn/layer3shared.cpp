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

#include "layer3shared.h"

static const unsigned char MPEG3_VER1 = 3;
static const unsigned char MPEG3_VER2 = 2;
static const unsigned char MPEG3_VER2_5 = 0;

double layer3_mpegsamplerate[2][4] =
{
    {22.05, 24, 16, 0},
    {44.1, 48, 32, 0}
};

int layer3_mpegbitrate[2][3][15] =
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

int ParseMP3header(unsigned int hdr, MPEGAUDIOHDR* pmah)
{
    if ((hdr & 0xffe00000) != 0xffe00000)
    {
        return 0;
    }

    pmah->layer = static_cast<unsigned char>(4 - ((hdr >> 17) & 3));
    pmah->crc = static_cast<unsigned char>(!((hdr >> 16) & 1));
    pmah->bitrateindex = static_cast<unsigned char>((hdr >> 12) & 15);
    pmah->padded = static_cast<unsigned char>((hdr >> 9) & 0x1);
    pmah->mode = static_cast<unsigned char>((hdr >> 6) & 0x3);
    pmah->modeext = static_cast<unsigned char>((hdr >> 4) & 0x3);

    if (pmah->layer == 4)
    {
        return 0;
    }

    if (pmah->bitrateindex == 15)
    {
        return 0;
    }

    pmah->version = static_cast<unsigned char>((hdr >> 19) & 3);

    if (pmah->version == 1)
    {
        return 0;
    }

    pmah->samplerateindex = static_cast<unsigned char>((hdr >> 10) & 0x3);

    if (pmah->samplerateindex == 3)
    {
        return 0;
    }

    pmah->channels = static_cast<unsigned char>((pmah->mode == 3) ? 1 : 2);
    pmah->samplerate = static_cast<unsigned short>(layer3_mpegsamplerate[1][pmah->samplerateindex] * 1000.0);

    if (pmah->version == MPEG3_VER2)
    {
        pmah->samplerate >>= 1;
    }

    if (pmah->version == MPEG3_VER2_5)
    {
        pmah->samplerate >>= 2;
    }

    if (!pmah->bitrateindex)
    {
        return 0;
    }

    if (pmah->version == MPEG3_VER1)
    {
        pmah->bitrate = static_cast<unsigned int>(layer3_mpegbitrate[1][pmah->layer - 1][pmah->bitrateindex]);
    }
    else
    {
        pmah->bitrate = static_cast<unsigned int>(layer3_mpegbitrate[0][pmah->layer - 1][pmah->bitrateindex]);
    }

    if (pmah->layer == 1)
    {
        pmah->framebytes = static_cast<unsigned short>((12000 * pmah->bitrate) / pmah->samplerate);
        pmah->framebytes = static_cast<unsigned short>((pmah->framebytes + pmah->padded) << 2);
        pmah->numframes = 384;
    }
    else
    {
        pmah->numframes = 1152;
        pmah->framebytes = static_cast<unsigned short>((144000 * pmah->bitrate) / pmah->samplerate);

        if (pmah->layer == 3 && pmah->version != MPEG3_VER1)
        {
            pmah->framebytes >>= 1;
            pmah->numframes = 576;
        }

        if (pmah->padded)
        {
            pmah->framebytes++;
        }
    }

    return pmah->framebytes;
}
