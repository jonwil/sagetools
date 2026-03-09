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

#ifndef PUTBITS_H
#define PUTBITS_H

class BitPut
{
public:
    BitPut() : mBitPosition(0) {}
    ~BitPut() {}

    void PutBits(unsigned int data, int numBits)
    {
        data <<= (32 - numBits);
        data >>= (32 - numBits);

        while (numBits > 0)
        {
            int bufferIndex = mBitPosition >> 3;
            int availableBits = 8 - (mBitPosition & 7);

            if (availableBits == 8)
            {
                mBitBuffer[bufferIndex] = 0;
            }

            int bitsSet;

            if (numBits > availableBits)
            {
                bitsSet = availableBits;
            }
            else
            {
                bitsSet = numBits;
            }

            mBitBuffer[bufferIndex] |= data >> (numBits - bitsSet) << (availableBits - bitsSet);
            mBitPosition += bitsSet;
            numBits -= bitsSet;
        }
    }

    unsigned int GetBitPosition() { return mBitPosition; }

    int GetBytes(void* pDst, int maxBytes)
    {
        int bufferBytes = mBitPosition >> 3;
        int copyBytes = bufferBytes < maxBytes ? bufferBytes : maxBytes;
        memcpy(pDst, mBitBuffer, copyBytes);
        memmove(mBitBuffer, &mBitBuffer[copyBytes], bufferBytes - copyBytes + 1);
        mBitPosition -= 8 * copyBytes;
        return copyBytes;
    }

private:
    unsigned int mBitPosition;
    unsigned char mBitBuffer[1025];
};

#endif
