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

#ifndef DECEALAYER32_H
#define DECEALAYER32_H

#include "decoder.h"
#include "decoders\decealayer31.h"

class EaLayer32PcmDec : public EaLayer3DecBase
{
public:
    static const Guid GUID = 'L32P';

    static DecoderDesc* GetDecoderDesc();

private:
    static DecoderDesc sDecoderDesc;

    static bool CreateInstanceEvent(Decoder* pDecoder);
};

class EaLayer32SpikeDec : public EaLayer3DecBase
{
public:
    static const Guid GUID = 'L32S';

    static DecoderDesc* GetDecoderDesc();

private:
    static DecoderDesc sDecoderDesc;

    static bool CreateInstanceEvent(Decoder* pDecoder);
};

#endif
