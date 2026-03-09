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

#include "private\mpegcommon.h"

const HuffEntry gHuffTableCount0[] =
{
    { 11, 6},{ 15, 6},{ 13, 6},{ 14, 6},{ 7, 6},{ 5, 6},{ 9, 5},{ 9, 5},
    { 6, 5},{ 6, 5},{ 3, 5},{ 3, 5},{ 10, 5},{ 10, 5},{ 12, 5},{ 12, 5},
    { 2, 4},{ 2, 4},{ 2, 4},{ 2, 4},{ 1, 4},{ 1, 4},{ 1, 4},{ 1, 4},
    { 4, 4},{ 4, 4},{ 4, 4},{ 4, 4},{ 8, 4},{ 8, 4},{ 8, 4},{ 8, 4},
    { 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},
    { 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},
    { 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},
    { 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},{ 0, 1},
};

const HuffEntry gHuffTableCount1[] =
{
    { 15, 4},{ 14, 4},{ 13, 4},{ 12, 4},{ 11, 4},{ 10, 4},{ 9, 4},{ 8, 4},
    { 7, 4},{ 6, 4},{ 5, 4},{ 4, 4},{ 3, 4},{ 2, 4},{ 1, 4},{ 0, 4},
};

const unsigned char CMpegLayer3Base::sHuffTableLinearBits[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 8, 10, 13, 4, 5, 6, 7, 8, 9, 11, 13 };

HuffCountTable CMpegLayer3Base::sHuffCountTables[2] =
{
    { 0, 6, 32 - 6 },
    { 0, 4, 32 - 4 },
};
