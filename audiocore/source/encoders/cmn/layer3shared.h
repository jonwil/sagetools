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

#ifndef LAYER3SHARED_H
#define LAYER3SHARED_H

struct MPEGAUDIOHDR
{
    unsigned int bitrate;
    unsigned short samplerate;
    unsigned short numframes;
    unsigned short framebytes;
    unsigned short padshort;
    unsigned char version;
    unsigned char layer;
    unsigned char channels;
    unsigned char mode;
    unsigned char modeext;
    unsigned char crc;
    unsigned char padded;
    unsigned char samplerateindex;
    unsigned char bitrateindex;
    char padchar[3];
};

int ParseMP3header(unsigned int hdr, MPEGAUDIOHDR* pmah);

#endif
