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

#ifndef SMP3TOEA_H
#define SMP3TOEA_H

class Bit_Reserve
{
public:
    Bit_Reserve();
    ~Bit_Reserve();
    void reset();
    unsigned int hsstell() const;
    unsigned int hgetbits(unsigned int N);
    void hputbuf(int val);
    void rewindNbytes(int N);
    unsigned int mInPtr;
    unsigned int mOutPtr;
    unsigned int mCached;
    unsigned int mCacheData;
    unsigned char mBuffer[2048];
};

class MP3toEA
{
public:
    MP3toEA();
    ~MP3toEA();
    void reset();
    int parse(unsigned char* buffer);
    int getgranulecount();
    void getgranule(int num, unsigned char*& buffer, int& len);
private:
    unsigned GetBits(unsigned n);
    void PutBits(int granule, unsigned val, unsigned n);
    Bit_Reserve mBitRes;
    int mFrameStart;
    int mGranuleCount;
    unsigned mInBitPtr;
    unsigned mOutBitPtr[2];
    unsigned char* mInBuffer;
    unsigned char mOutBuffer[2][2048];
};

#endif
