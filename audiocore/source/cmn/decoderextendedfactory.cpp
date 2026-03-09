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

#include "decoderregistry.h"

DecoderExtended* DecoderRegistry::DecoderExtendedFactory(DecoderHandle decoderHandle, unsigned int numChannels, unsigned int maxRequests, System* pSystem)
{
    DecoderExtended* pDecoderEx = DecoderExtended::CreateInstance(pSystem, numChannels);

    if (!pDecoderEx)
    {
        goto abort;
    }

    DecoderRegistry* pDecoderRegistry;
    pDecoderRegistry = pSystem->GetDecoderRegistry();

    if (!pDecoderRegistry)
    {
        goto abort;
    }

    Decoder* pDecoder;
    pDecoder = pDecoderRegistry->DecoderFactory(decoderHandle, numChannels, maxRequests, pSystem);

    if (!pDecoder)
    {
        goto abort;
    }

    pDecoderEx->SetDecoder(pDecoder);
    return pDecoderEx;

abort:
    if (pDecoderEx)
    {
        pDecoderEx->Release();
    }

    return 0;
}
