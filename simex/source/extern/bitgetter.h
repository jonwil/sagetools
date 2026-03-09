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

#ifndef BITGETTER_H
#define BITGETTER_H

class BitGetter
{
public:
    BitGetter() {}
    ~BitGetter() {}

    void SetBitBuffer(void* pBuffer)
    {
        mpBitBuffer = (unsigned char*)pBuffer;
        mBitPosition = 0;
    }

    unsigned int GetBitPosition() { return mBitPosition; }

    unsigned int GetBits(unsigned int numBits)
    {
        int val = 0;

        while (numBits > 0)
        {
            unsigned int index = mBitPosition >> 3;
            unsigned int bitpos = mBitPosition % 8;
            unsigned int nbits = 8 - bitpos;

            if (nbits > numBits)
            {
                nbits = numBits;
            }

            val = (val << nbits) | (((int)mpBitBuffer[index] >> (8 - bitpos - nbits)) & ((1 << nbits) - 1));
            mBitPosition += nbits;
            numBits -= nbits;
        }

        return val;
    }

private:
    unsigned char* mpBitBuffer;
    unsigned int mBitPosition;
};

#endif
