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

#include "encoders\mp3toea.h"

static const unsigned bitrate_table[2][15] =
{
    { 0,32,40,48,56,64,80,96,112,128,160,192,224,256,320 },
    { 0,8,16,24,32,40,48,56,64,80,96,112,128,144,160 }
};

static const unsigned samprate_table[3][3] =
{
    { 11025, 12000,  8000 },
    { 22050, 24000, 16000 },
    { 44100, 48000, 32000 }
};

unsigned MP3toEA::GetBits(unsigned n)
{
    unsigned val = 0;

    while (n > 0)
    {
        unsigned index = mInBitPtr / 8;
        unsigned bitpos = mInBitPtr % 8;
        unsigned nbits = 8 - bitpos;
        if (nbits > n) nbits = n;
        val = (val << nbits) | ((mInBuffer[index] >> (8 - bitpos - nbits)) & ((1 << nbits) - 1));
        mInBitPtr += nbits;
        n -= nbits;
    }

    return val;
}

void MP3toEA::PutBits(int granule, unsigned val, unsigned n)
{
    unsigned index = mOutBitPtr[granule] / 8;
    unsigned bitpos = mOutBitPtr[granule] % 8;
    val = ((mOutBuffer[granule][index] & (0x100 - (1 << (8 - bitpos)))) << 24) | (val << (32 - n - bitpos));
    mOutBuffer[granule][index] = static_cast<unsigned char>(val >> 24);
    mOutBuffer[granule][index + 1] = static_cast<unsigned char>((val >> 16) & 0xff);
    mOutBuffer[granule][index + 2] = static_cast<unsigned char>((val >> 8) & 0xff);
    mOutBuffer[granule][index + 3] = static_cast<unsigned char>(val & 0xff);
    mOutBitPtr[granule] += n;
}

void MP3toEA::reset()
{
    mBitRes.reset();
    mFrameStart = 0;
}

int MP3toEA::parse(unsigned char* buffer)
{
    unsigned gr;
    unsigned ch;
    unsigned i;
    mGranuleCount = 0;
    mInBitPtr = 0;
    mOutBitPtr[0] = 0;
    mOutBitPtr[1] = 0;
    mInBuffer = buffer;
    unsigned sync = GetBits(11);
    unsigned idbits = GetBits(2);
    unsigned layer = GetBits(2);
    unsigned errprot = GetBits(1);
    unsigned bitrateindex = GetBits(4);
    unsigned samprateindex = GetBits(2);
    unsigned padding = GetBits(1);
    GetBits(1);
    unsigned mode = GetBits(2);
    unsigned modeext = GetBits(2);
    GetBits(4);

    if ((sync != 0x7ff) || (idbits == 1) || (layer == 0) || (bitrateindex == 15) || (samprateindex == 3))
    {
        return -1;
    }

    if (layer != 1)
    {
        return -2;
    }

    unsigned version = idbits;

    if (version > 0)
    {
        version--;
    }

    unsigned lsf = static_cast<unsigned int>(version < 2);
    unsigned bitrate = bitrate_table[lsf][bitrateindex];
    unsigned samprate = samprate_table[version][samprateindex];
    unsigned framesize = (144000 * bitrate) / samprate;

    if (lsf)
    {
        framesize >>= 1;
    }

    framesize += padding;
    unsigned slots = framesize - 4;
    PutBits(0, idbits, 2);
    PutBits(0, samprateindex, 2);
    PutBits(0, mode, 2);
    PutBits(0, modeext, 2);
    PutBits(0, 0, 1);
    PutBits(1, idbits, 2);
    PutBits(1, samprateindex, 2);
    PutBits(1, mode, 2);
    PutBits(1, modeext, 2);
    PutBits(1, 1, 1);

    if (errprot == 0)
    {
        GetBits(16);
        slots -= 2;
    }

    unsigned backpointer;
    unsigned chanbits;
    unsigned numgran;
    unsigned numchan;
    unsigned len[2][2];

    if (lsf)
    {
        backpointer = GetBits(8);
        chanbits = 63;
        numgran = 1;

        if (mode == 3)
        {
            numchan = 1;
            GetBits(1);
            slots -= 9;
        }
        else
        {
            numchan = 2;
            GetBits(2);
            slots -= 17;
        }
    }
    else
    {
        backpointer = GetBits(9);
        chanbits = 59;
        numgran = 2;

        if (mode == 3)
        {
            numchan = 1;
            GetBits(5);
            PutBits(1, GetBits(4), 4);
            slots -= 17;
        }
        else
        {
            numchan = 2;
            GetBits(3);
            PutBits(1, GetBits(8), 8);
            slots -= 32;
        }
    }

    for (gr = 0; gr < numgran; gr++)
    {
        for (ch = 0; ch < numchan; ch++)
        {
            len[gr][ch] = GetBits(12);
            PutBits(static_cast<int>(gr), len[gr][ch], 12);
            PutBits(static_cast<int>(gr), GetBits(20), 20);
            PutBits(static_cast<int>(gr), GetBits(20), 20);
            PutBits(static_cast<int>(gr), GetBits(chanbits - 52), chanbits - 52);
        }
    }

    for (i = 0; i < slots; i++)
    {
        mBitRes.hputbuf(static_cast<int>(GetBits(8)));
    }

    unsigned main_data_end = mBitRes.hsstell() / 8;

    if (unsigned flush_main = (mBitRes.hsstell() % 8))
    {
        mBitRes.hgetbits(8 - flush_main);
        main_data_end++;
    }

    int bytes_to_discard = static_cast<int>(mFrameStart - main_data_end - backpointer);
    mFrameStart += slots;

    if (bytes_to_discard < 0)
    {
        return -3;
    }

    if (main_data_end > 2048)
    {
        mFrameStart -= 2048;
        mBitRes.rewindNbytes(2048);
    }

    for (; bytes_to_discard > 0; bytes_to_discard--)
    {
        mBitRes.hgetbits(8);
    }

    for (gr = 0; gr < numgran; gr++)
    {
        for (ch = 0; ch < numchan; ch++)
        {
            unsigned part_2_3_len = len[gr][ch];
            while (part_2_3_len > 0)
            {
                unsigned n = part_2_3_len;
                if (n > 24) n = 24;
                PutBits(static_cast<int>(gr), mBitRes.hgetbits(n), n);
                part_2_3_len -= n;
            }
        }
    }

    mGranuleCount = static_cast<int>(numgran);
    return  static_cast<int>(framesize);
}

int MP3toEA::getgranulecount()
{
    return mGranuleCount;
}

int MP3toEA::getgranulesize(int index)
{
    int length = 0;

    if (0 <= index && index < mGranuleCount)
    {
        length = static_cast<int>((mOutBitPtr[index] + 7) / 8);
    }

    return length;
}

void MP3toEA::getgranule(int num, unsigned char*& buffer, int& len)
{
    if (num < mGranuleCount)
    {
        buffer = mOutBuffer[num];
        len = static_cast<int>((mOutBitPtr[num] + 7) / 8);
    }
    else
    {
        len = 0;
    }
}
