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

#ifndef SAMPLEBUFFER_H
#define SAMPLEBUFFER_H

class System;

class SampleBuffer
{
public:
    static unsigned int GetSize(unsigned int numChannels, unsigned int maxLocks, unsigned int maxSamples, unsigned int* pAlignment, System* pSystem);
    static SampleBuffer* CreateInstance(unsigned int numChannels, unsigned int maxLocks, unsigned int maxSamples, void* pMem, void* pStorage, System* pSystem);
    static unsigned int CalculateStorageSize(unsigned int numChannels, unsigned int maxSamples) { return numChannels * maxSamples * sizeof(float); }
    static unsigned int CalculateSpuStorageSize(unsigned int maxLocks, unsigned int maxSamples) { return maxLocks * maxSamples * sizeof(float); }
    void SetStorage(void* pMem) { mpStorage = static_cast<float*>(pMem); }
    void SetMaxSamples(unsigned int maxSamples) { mMaxSamples = static_cast<unsigned short>(maxSamples); }
    float* LockChannel(unsigned int channel) { return mpStorage + (channel * mMaxSamples); }
    void UnlockChannel(unsigned int channel) {}
    void SetNumSamples(unsigned int numSamples) { mNumSamples = static_cast<unsigned short>(numSamples); }
    unsigned int GetNumSamples() { return mNumSamples; }
    unsigned int GetMaxSamples() { return mMaxSamples; }
    unsigned int GetMaxLocks() { return mNumChannels; }

private:
    System* mpSystem;
    float* mpStorage;
    float* mpTempStore;
    unsigned short mNumSamples;
    unsigned short mMaxSamples;
    unsigned char mNumChannels;
};

#endif
