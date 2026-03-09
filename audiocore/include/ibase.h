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

#ifndef IBASE_H
#define IBASE_H

inline void LinearAllocAddSize(unsigned int& totalSize, unsigned int size, unsigned int alignment = 8)
{
    totalSize = AlignUp(totalSize, alignment);
    totalSize += size;
}

template<class TAddr, class TMemBlock>
inline void LinearAlloc(TAddr& addr, TMemBlock& memBlock, unsigned int size, unsigned int alignment = 8)
{
    addr = (TAddr)AlignUp(memBlock, alignment);
    memBlock = (TMemBlock)((uintptr_t)addr + size);
}

#endif
