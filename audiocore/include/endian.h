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

#ifndef ENDIAN_H
#define ENDIAN_H

class ENDIAN
{
private:
    template <class tdest, class tsrc> static void inline Reverse8_8(tdest& dst, tsrc& src)
    {
        (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[7];
        (reinterpret_cast<unsigned char*>(&dst))[1] = (reinterpret_cast<const unsigned char*>(&src))[6];
        (reinterpret_cast<unsigned char*>(&dst))[2] = (reinterpret_cast<const unsigned char*>(&src))[5];
        (reinterpret_cast<unsigned char*>(&dst))[3] = (reinterpret_cast<const unsigned char*>(&src))[4];
        (reinterpret_cast<unsigned char*>(&dst))[4] = (reinterpret_cast<const unsigned char*>(&src))[3];
        (reinterpret_cast<unsigned char*>(&dst))[5] = (reinterpret_cast<const unsigned char*>(&src))[2];
        (reinterpret_cast<unsigned char*>(&dst))[6] = (reinterpret_cast<const unsigned char*>(&src))[1];
        (reinterpret_cast<unsigned char*>(&dst))[7] = (reinterpret_cast<const unsigned char*>(&src))[0];
    }

    template <class tdest, class tsrc> static void inline Reverse4_8(tdest& dst, tsrc& src)
    {
        (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[3];
        (reinterpret_cast<unsigned char*>(&dst))[1] = (reinterpret_cast<const unsigned char*>(&src))[2];
        (reinterpret_cast<unsigned char*>(&dst))[2] = (reinterpret_cast<const unsigned char*>(&src))[1];
        (reinterpret_cast<unsigned char*>(&dst))[3] = (reinterpret_cast<const unsigned char*>(&src))[0];
    }

    template <class tdest, class tsrc> static void inline Reverse4_4(tdest& dst, tsrc& src)
    {
        (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[3];
        (reinterpret_cast<unsigned char*>(&dst))[1] = (reinterpret_cast<const unsigned char*>(&src))[2];
        (reinterpret_cast<unsigned char*>(&dst))[2] = (reinterpret_cast<const unsigned char*>(&src))[1];
        (reinterpret_cast<unsigned char*>(&dst))[3] = (reinterpret_cast<const unsigned char*>(&src))[0];
    }

    template <class tdest, class tsrc> static void inline Reverse2_2(tdest& dst, tsrc& src)
    {
        (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[1];
        (reinterpret_cast<unsigned char*>(&dst))[1] = (reinterpret_cast<const unsigned char*>(&src))[0];
    }

    template <class tdest, class tsrc> static void inline Reverse2_4(tdest& dst, tsrc& src)
    {
        (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[1];
        (reinterpret_cast<unsigned char*>(&dst))[1] = (reinterpret_cast<const unsigned char*>(&src))[0];
    }

    template <class tdest, class tsrc> static void inline Reverse(tdest& dst, tsrc& src)
    {
        if (sizeof(tsrc) == 1 && sizeof(tdest) == 1)
        {
            (reinterpret_cast<unsigned char*>(&dst))[0] =
                (reinterpret_cast<const unsigned char*>(&src))[0];
        }
        else if (sizeof(tsrc) == 2 && sizeof(tdest) == 2)
        {
            Reverse2_2(dst, src);
        }
        else if (sizeof(tsrc) == 2 && sizeof(tdest) == 4)
        {
            Reverse2_4(dst, src);
        }
        else if (sizeof(tsrc) == 4 && sizeof(tdest) == 4)
        {
            Reverse4_4(dst, src);
        }
        else if (sizeof(tsrc) == 4 && sizeof(tdest) == 8)
        {
            Reverse4_4(dst, src);
        }
        else if (sizeof(tsrc) == 8 && sizeof(tdest) == 8)
        {
            Reverse8_8(dst, src);
        }
    }

    template <class tdest, class tsrc> static void inline Copy(tdest& dst, tsrc src)
    {
        if (sizeof(tdest) == 1)
        {
            (reinterpret_cast<unsigned char*>(&dst))[0] = (reinterpret_cast<const unsigned char*>(&src))[0];
        }
        else if (sizeof(tdest) == 2)
        {
            (reinterpret_cast<unsigned short*>(&dst))[0] = (reinterpret_cast<const unsigned short*>(&src))[0];
        }
        else if (sizeof(tdest) == 4 && sizeof(tsrc) == 2)
        {
            (reinterpret_cast<unsigned int*>(&dst))[0] = (reinterpret_cast<const unsigned short*>(&src))[0];
        }
        else if (sizeof(tdest) == 4 && sizeof(tsrc) == 4)
        {
            (reinterpret_cast<unsigned int*>(&dst))[0] = (reinterpret_cast<const unsigned int*>(&src))[0];
        }
        else if (sizeof(tdest) == 4 && sizeof(tsrc) == 8)
        {
            (reinterpret_cast<unsigned int*>(&dst))[0] = (reinterpret_cast<const unsigned int*>(&src))[0];
        }
        else if (sizeof(tdest) == 8 && sizeof(tsrc) == 8)
        {
            (reinterpret_cast<unsigned short*>(&dst))[0] = (reinterpret_cast<const unsigned short*>(&src))[0];
        }
    }

public:
    template <class tdest, class tsrc> static void inline PutB(tdest& dst, tsrc src)
    {
        Reverse(dst, src);
    }

    template <class tdest, class tsrc> static void inline PutL(tdest& dst, tsrc src)
    {
        Copy(dst, src);
    }

    template <class tdest, class tsrc> static void inline PutUB(tdest& dst, tsrc &src)
    {
        Reverse(dst, src);
    }

    template <class dstT, class srcT> static void inline Put(bool bigendian, dstT& dst, srcT src)
    {
        if (bigendian == true)
        {
            PutB(dst, src);
        }
        else
        {
            PutL(dst, src);
        }
    }
};

#endif
