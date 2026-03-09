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

#include "encoderregistry.h"

EncoderExtended* EncoderRegistry::EncoderExtendedFactory(EncoderHandle encoderHandle, int numChannels, int sampleRate, System* pSystem)
{
    EncoderExtended* pEncoderEx = EncoderExtended::CreateInstance(pSystem);

    if (!pEncoderEx)
    {
        return pEncoderEx;
    }

    EncoderDesc* pEncoderDesc = static_cast<EncoderDesc*>(encoderHandle);
    pEncoderEx->SetEncoder(pEncoderDesc->CreateInstance(numChannels, sampleRate, pSystem));
    pEncoderEx->GetEncoder()->SetSystem(pSystem);
    pEncoderEx->GetEncoder()->SetChannels(numChannels);
    pEncoderEx->GetEncoder()->SetSampleRate(sampleRate);
    pEncoderEx->GetEncoder()->SetEncoderDesc(pEncoderDesc);
    return pEncoderEx;
}
