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

#ifndef CODA_H
#define CODA_H

#include <stddef.h>

typedef void* (*CODANewFunctionType)(size_t byteSize);
typedef void (*CODADeleteFunctionType)(void* voidPtr);
void CODASetNew(CODANewFunctionType pNew);
void CODASetDelete(CODADeleteFunctionType pDelete);

typedef struct XA16STATE
{
    short sample1;
    short sample2;
} XA16STATE;

typedef struct MXAPACKET16
{
    int numframes;
    short sample1;
    short sample2;
    unsigned char* psrc;
    short* pdst;
} MXAPACKET16;

typedef struct EAXAVARS16
{
    int residual;
    int numsamples;
    int sampledatasize;
    short* presidue;
    short xablock[30];
} EAXAVARS16;

class CEAXABLKDec
{
public:
    CEAXABLKDec();
    ~CEAXABLKDec() {}
    void* operator new(size_t size);
    void operator delete(void* ptr);
    int Feed(void* pSampleData, int sampleDataSize, int numSamples);
    int Decode(short* pDstBuf[], int numSamples);
    XA16STATE GetState();
    void SetState(XA16STATE* pxav);

private:
    EAXAVARS16 xav;
    MXAPACKET16 xap;
};

class CShortDestDecoder
{
public:
    virtual int Feed(void* pSampleData, int sampleDataSize, int numSamples) = 0;
    virtual int Decode(short* pDstBuf[], int samples) = 0;
    virtual void SetState(short* pchannel) = 0;
};

class CSign24IntDecS16 : public CShortDestDecoder
{
public:
    CSign24IntDecS16();
    virtual ~CSign24IntDecS16() {}
    void* operator new(size_t size);
    void operator delete(void* ptr);
    virtual int Feed(void* pSampleData, int sampleDataSize, int numSamples);
    virtual int Decode(short* pDstBuf[], int samples);
    int GetState();
    virtual void SetState(short* pchannel);

private:
    unsigned char* mpSrc;
    short* mpDst;
    int mRemainingSamples;
    int mSampleDataSize;
    short mChannels;
};

extern CODANewFunctionType CODANew;
extern CODADeleteFunctionType CODADelete;

#endif

