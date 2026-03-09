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

#ifndef FTOI_H
#define FTOI_H

namespace FToI
{
    static inline int Round(float val)
    {
        int result;

        __asm
        {
            fld     dword ptr val
            fistp   dword ptr result
        }

        return result;
    }

    static inline int Fast(float val)
    {
        return Round(val);
    }
}

#endif
