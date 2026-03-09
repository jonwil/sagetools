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

#ifndef ENCODER_H
#define ENCODER_H

#include "system.h"

struct EncoderDesc
{
    Guid guid;
    Encoder* (*CreateInstance)(int numChannels, int sampleRate, System* pSystem);
    void* listNode;
    unsigned short maxLatency;
    unsigned char seekDataVersion;
    bool isSeekable;
};

class Encoder
{
public:
    virtual int Encode(float* pSampleData, unsigned char* pEncodedData, int numSamplesIn, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes) = 0;
    virtual int Flush(unsigned char* pEncodedData, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes) = 0;
    virtual int GetDataRateOverhead();
    float GetAverageDataRate();
    int GetEncodeMemoryRequired(int numSamples);
    virtual int GetSeekMemoryRequired(int numSamples);
    bool IsSeekable() { return mpEncoderDesc->isSeekable; };
    int GetSeekDataVersion() { return mpEncoderDesc->seekDataVersion; };
    unsigned short GetMaxLatency() { return mpEncoderDesc->maxLatency; };
    void SetVbrQuality(float quality);

    void SetCbrRate(int bitsPerSecond)
    {
        mCbrRate = bitsPerSecond;
        mBitRateManagement = BITRATEMANAGEMENT_USINGCBR;
    }

    virtual void Release();

protected:
    enum BitRateManagement
    {
        BITRATEMANAGEMENT_USINGVBR = 0,
        BITRATEMANAGEMENT_USINGCBR = 1
    };

    void SetSystem(System* pSystem) { mpSystem = pSystem; }
    System* GetSystem() { return mpSystem; }
    int GetChannels() { return mNumChannels; }
    int GetSampleRate() { return mSampleRate; }
    BitRateManagement GetBitRateManagement() { return static_cast<BitRateManagement>(mBitRateManagement); }
    Encoder() {};
    virtual ~Encoder() {};

    float mAverageDataRate;
    float mVbrQuality;
    int mCbrRate;

private:
    friend class EncoderRegistry;
    friend class EncoderExtended;
    friend class System;

    void SetChannels(int numChannels) { mNumChannels = static_cast<unsigned char>(numChannels); }
    void SetSampleRate(int sampleRate) { mSampleRate = sampleRate; }
    void SetEncoderDesc(EncoderDesc* pEncoderDesc) { mpEncoderDesc = pEncoderDesc; }

    System* mpSystem;
    EncoderDesc* mpEncoderDesc;
    int mSampleRate;
    unsigned char mNumChannels;
    unsigned char mBitRateManagement;
};

class EncoderExtended
{
public:
    int Encode(float* pSampleData, unsigned char* pEncodedData, int numSamples, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    int Encode(short* pSampleData[], unsigned char* pEncodedData, int numSamples, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    int Flush(unsigned char* pEncodedData, int* pBytesEncoded, void* pSeekData, int* pSeekDataBytes);
    int GetDataRateOverhead();
    float GetAverageDataRate();
    int GetEncodeMemoryRequired(int numFrames);
    int GetSeekMemoryRequired(int numFrames);
    bool IsSeekable();
    int GetSeekDataVersion() { return mpEncoder->GetSeekDataVersion(); };
    unsigned short GetMaxLatency() { return mpEncoder->GetMaxLatency(); };
    void SetVbrQuality(float quality);
    void SetCbrRate(int bitsPerSecond);
    void Release();

protected:
    static void TranslateS16ToF32(short* pSrc[], float* pDst, int channels, int numSamples);
    void SetEncoder(Encoder* encoder) { mpEncoder = encoder; }
    Encoder* GetEncoder() { return mpEncoder; }

private:
    friend class EncoderRegistry;

    static EncoderExtended* CreateInstance(System* pSystem);
    ~EncoderExtended() {};

    Encoder* mpEncoder;
};

#endif
