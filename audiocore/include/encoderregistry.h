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

#ifndef ENCODERREGISTRY_H
#define ENCODERREGISTRY_H

#include "encoder.h"
#include "private\linklist.h"

class EncoderRegistry
{
public:
    typedef void* EncoderHandle;

    void RegisterAllEncoders();
    EncoderHandle GetEncoderHandle(Guid guid);
    Encoder* EncoderFactory(EncoderHandle encoderHandle, int numChannels, int sampleRate, System* pSystem = System::GetInstance());
    EncoderExtended* EncoderExtendedFactory(EncoderHandle encoderHandle, int numChannels, int sampleRate, System* pSystem = System::GetInstance());
    EncoderHandle RegisterEncoder(EncoderDesc* pEncoderDesc);

private:
    friend class System;

    EncoderDesc* GetEncoderDescFromNode(ListNode* pNode) { return reinterpret_cast<EncoderDesc*>(reinterpret_cast<char*>(pNode) - (size_t) & reinterpret_cast<const volatile char&>((((EncoderDesc*)0)->listNode))); }
    static EncoderRegistry* CreateInstance(System* pSystem = System::GetInstance());

    void Release();
    ListQueue mEncoderDescList;
    System* mpSystem;
};

#endif
