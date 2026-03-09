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

#ifndef MPEGCOMMON_H
#define MPEGCOMMON_H

enum BlockType
{
    BLOCKTYPE_SHORT = 2
};

struct HuffEntry
{
    unsigned char value;
    unsigned char length;
};

struct HuffTable
{
    unsigned short tableSize;
    short* pEntries;
};

struct HuffCountTable
{
    const HuffEntry* pEntries;
    unsigned short maxCodeBits;
    unsigned short maxCodeShifter;
};

struct SBI
{
    short l[23];
    unsigned char s[14];
};

struct GranuleInfo
{
    unsigned short part2And3Length;
    unsigned short bigValues;
    unsigned short scaleFacCompress;
    unsigned char globalGain;
    unsigned char windowSwitchingFlag;
    unsigned char blockType;
    unsigned char mixedBlockFlag;
    unsigned char region0Count;
    unsigned char region1Count;
    unsigned char tableSelect[3];
    unsigned char count1TableSelect;
    unsigned char subBlockGain[3];
    unsigned char preFlag;
    unsigned int scaleFacScale;
};

struct Layer3SideInfo
{
    unsigned int mainDataBegin;
    struct
    {
        unsigned char scfsi[4];
    } ch[2];
};

struct Layer3ScaleFactors
{
    short longBlock[23];
    short shortBlock[3][13];
};

struct AntiAliasCoefficients
{
    float cs[8];
    float ca[8];
};

extern const HuffEntry gHuffTableCount0[];
extern const HuffEntry gHuffTableCount1[];

class Bit_Reserve
{
public:
    Bit_Reserve() { reset(); }
    ~Bit_Reserve() {}
    void reset();
    unsigned int hsstell() const;
    unsigned int hgetbits(unsigned int N);
    unsigned int hget1bit();
    void hputbuf(int val);
    void rewindNbits(int N);
    void rewindNbytes(int N);

private:
    unsigned int mInPtr;
    unsigned int mOutPtr;
    unsigned int mCached;
    unsigned int mCacheData;
    unsigned char mBuffer[2048];
};

class CMpegBase
{
public:
    CMpegBase();
    virtual ~CMpegBase();
    int ProcessHeader(unsigned int hdr);
    int Close();
    void Seek(void* buf);

    int Real_mSampFreq;
    int mSampFreq;
    int mBitRate;
    int cFrameSize;
    unsigned short mFrameSamples;
    unsigned char mLayer;
    unsigned char mOpened;
    unsigned char mChannels;
    unsigned int mHeader;

protected:
    void Reset();
    int DecodeHeader();
    int AllocateSynth(int numChannels);
    void FreeSynth();
    void ResetSynth();
    void PolySynth(int channel, float* pOutputSamples, float* pSubBand);
    void PolySynthSSE(int channel, float* pOutputSamples, float* pSubBand);

    unsigned char* mBufPtr;
    unsigned char* mBufPtrBase;
    unsigned char* mBufPtrNext;
    unsigned int mShiftReg;
    int mShiftRegBits;
    int mResetSynthRequired;
    static const unsigned short sSampleRateTable[];
    unsigned char mBandOffset[2];
    unsigned char mMPEG25;
    unsigned char mLSF;
    unsigned char sfreq;
    unsigned char max_gr;
    unsigned char mVersion;
    unsigned char mErrorProt;
    unsigned char mBitRateIdx;
    unsigned char mSampFreqIdx;
    unsigned char mPadding;
    unsigned char mMode;
    unsigned char mModeExt;
    unsigned char mCopyright;
    unsigned char mOriginal;
    static const unsigned char sNumSfbBlock[6][3][4];

    inline void LoadBitRegister()
    {
        while (mShiftRegBits <= 24)
        {
            mShiftReg |= *mBufPtr++ << (24 - mShiftRegBits);
            mShiftRegBits += 8;
        }
    }

    unsigned int GetBits(int n)
    {
        while (mShiftRegBits < n)
        {
            mShiftReg |= *mBufPtr++ << (24 - mShiftRegBits);
            mShiftRegBits += 8;
        }

        const unsigned int bits = (mShiftReg >> (32 - n));
        mShiftReg <<= n;
        mShiftRegBits -= n;
        return bits;
    }

    int GetHeader();

    typedef float PolySynthHistoryF[2][0x120];
    typedef short PolySynthHistoryS[2][0x120];
    union
    {

        PolySynthHistoryF* mpPolySynthHistoryF;
        PolySynthHistoryS* mpPolySynthHistoryS;
    };

    union
    {

        PolySynthHistoryF* mpLoadedPolySynthHistoryF;
        PolySynthHistoryS* mpLoadedPolySynthHistoryS;
    };
};

struct Layer3Temp
{
    Layer3SideInfo sideInfo;
};

extern const float gTwoToNegativeQuarterPower[256];

class CMpegLayer3Base : public CMpegBase
{
public:
    CMpegLayer3Base();
    virtual ~CMpegLayer3Base();

    static const unsigned short SAMPLES_PER_GRANULE = 576;

protected:
    typedef float PrevBlockX4[32 / 4][18][4];
    HuffTable mHuffTables[32];
    static const unsigned char sHuffTableLinearBits[];
    static HuffCountTable sHuffCountTables[];
    static const char slen[2][16];
    static const struct SBI sfBandIndex[6];
    static const AntiAliasCoefficients sAntiAliasCoefficients;
    static const float sToPowerOf4over3Coefficients0To31[32];

    void SToPowerOf4over3(int SToPowerOf4over3Count, short* pSToPowerOf4over3Input, float* pSToPowerOf4over3Results);
    void I_Stereo_K_Values(unsigned int is_pos, unsigned int io_type, unsigned int i, float k[2][32 * 18]);
    void Dequantize(unsigned int ch, unsigned int gr, float samples[32 * 18]);
    void Reorder(unsigned int ch, unsigned int gr, float inputSamples[32 * 18], float outputSamples[32 * 18]);
    void Stereo(int gr, float samples[2][32][18]);
    void AntiAlias(unsigned int ch, unsigned int gr, float samples[32 * 18]);
    void Hybrid(int ch, int gr, float samples[32 / 4][18][4]);
    int AllocateHybrid(int numChannels);

    Layer3Temp* mpTemp;
    GranuleInfo mGranuleInfo[2][2];
    Layer3ScaleFactors mScaleFactors[2];
    const float* mpTwoToNegativeQuarterPower;
    const float* mpLoadedTwoToNegativeQuarterPower;
    short* mpLoadedTableSelect[3];
    PrevBlockX4* mpPrevBlockX4;
    PrevBlockX4* mpLoadedPrevBlockX4;
    typedef float K[2][32 * 18];
    unsigned int* mpIs_pos;
    float* mpIs_rat_io;
    K* mpK;
    int mFrameStart;
    unsigned short mTwoToNegativeQuarterPowerTableSize;

private:
    void HuffmanTableSetup();
};

class EALayer3Core : public CMpegLayer3Base
{
public:
    EALayer3Core() {};
    EALayer3Core(int numChannels);
    ~EALayer3Core();
    int ProcessEALayer3Header(unsigned int hdr);
    int Open(void* buf, int filesize);
    int Decode(float** ppOutputSamples);

protected:
    bool GetSideInfo(unsigned int gr);
    void GetScaleFactors(int ch, int gr);
    void GetLsfScaleData(int ch, int gr, unsigned char scaleFacBuf[54]);
    void GetLsfScaleFactors(int ch, int gr);
    void DecodeHuffman(int ch, int gr, float output[32 * 18], int part2_start);
    short GetBitsSafely(int n);
    unsigned GetBitPos();
    void RewindBits(int n);
};

void FrequencyInversionX4(float samples[32 / 4][18][4]);
void FrequencyInversionX4SSE(float samples[32 / 4][18][4]);
void ReorderForVectoring(float source[32 * 18], float dest[32 / 4][18][4]);
void ReorderForFPolySynth(float source[32 / 4][18][4], float dest[18][32]);
void ReorderForFPolySynthSSE(float source[32 / 4][18][4], float dest[18][32]);

#endif
