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

#include "coda\include\coda.h"
#include "simex\simex.h"
#include "cmn\isimex.h"
#include "cmn\fileio.h"
#include "coda\extern\ftoi.h"
#include "system.h"
#include "decoder.h"
#include "decoderregistry.h"

struct SSTREAM
{
    FileHandle* pFileHandle;
    unsigned char* pmem;
};

void* SIMEXI_New(unsigned int s)
{
    return Allocator::Alloc(s);
}

void SIMEXI_Delete(void* p)
{
    Allocator::Free(p);
}

int SIMEXCODAvectored;

void SIMEXI_vectorCODA()
{
    if (!SIMEXCODAvectored)
    {
        CODASetNew(SIMEXI_New);
        CODASetDelete(SIMEXI_Delete);
        SIMEXCODAvectored = 1;
    }
}

void SIMEXI_streamwrite(SSTREAM* pss, void* pdata, int size)
{
    if (pss->pFileHandle)
    {
        FileIO::Write(pss->pFileHandle, pdata, size);
    }
    else
    {
        memcpy(pss->pmem, pdata, size);
        pss->pmem += size;
    }
}

int SIMEXI_streamalign4(SSTREAM* pss)
{
    int bytes = 0;
    char val = 0;

    if (pss->pFileHandle)
    {
        while ((FileIO::Tell(pss->pFileHandle) & 3) != 0)
        {
            FileIO::Write(pss->pFileHandle, &val, 1);
            bytes++;
        }
    }
    else
    {
        while (((int)pss->pmem & 3) != 0)
        {
            *pss->pmem++ = val;
            bytes++;
        }
    }

    return bytes;
}

struct SNDIOPSAMPLEHDR
{
    unsigned char ver;
    unsigned char samplerep;
    char pad[2];
    int loopstart;
    int loopend;
    int totalframes;
};

int SIMEXI_writeiopheader(SSTREAM* pss, SSOUND* psound)
{
    if ((psound->playloc & 0x100) == 0)
    {
        return 0;
    }

    SNDIOPSAMPLEHDR ish;
    memset(&ish, 0, sizeof(ish));
    PutI(&ish.ver, 0, 1);
    PutI(&ish.samplerep, psound->samplerep, 1);
    PutI(&ish.totalframes, psound->length, 4);
    PutI(&ish.loopstart, psound->sustainstart, 4);
    PutI(&ish.loopend, psound->sustainend, 4);
    SIMEXI_streamwrite(pss, &ish, 16);
    return 16;
}

void SIMEXI_skipiopheader(FileHandle* pgs, SSOUND* pss)
{
    if ((pss->playloc & 0x100) != 0)
    {
        while ((FileIO::Tell(pgs) & 3) != 0)
        {
            FileIO::Seek(pgs, FileIO::Tell(pgs) + 1);
        }

        FileIO::Seek(pgs, FileIO::Tell(pgs) + 16);
    }
}

int SIMEXI_isbigendian()
{
    int val = 1;
    char* pchar = (char*)&val;

    if (*pchar == 0)
    {
        return 1;
    }

    return 0;
}

int simportSIGN24LIT_INTf(FileHandle* gs, __int64 sampleoffsets[], int startframedisk, int startframetrack, int numframes, int numchannels, int samplerep, int flags, SCOMPSTATE* pcompstate[], short* ptracks[], SSOUND* pss)
{
    int frame;
    int chan;
    pcompstate = pcompstate;
    CSign24IntDecS16* pDecoder = new CSign24IntDecS16();
    short shortNumChannels = numchannels;
    pDecoder->SetState(&shortNumChannels);
    int bufsize = 3 * numchannels;
    unsigned char* val = (unsigned char*)Allocator::Alloc(bufsize);
    short** pDstBuf = (short**)Allocator::Alloc(4 * numchannels);
    memset(pDstBuf, 0, 4 * numchannels);

    for (frame = startframetrack; frame < startframetrack + numframes; frame++)
    {
        if (!FileIO::Read(gs, val, bufsize))
        {
            SIMEXI_setlasterr("Couldn't read 16-bit samples from file.");
            return 0;
        }

        pDecoder->Feed(val, 2 * numchannels, 1);

        for (chan = 0; chan < numchannels; chan++)
        {
            pDstBuf[chan] = &ptracks[chan][frame];
        }

        pDecoder->Decode(pDstBuf, 1);

        if ((frame & 0xFFF) == 0)
        {
            float percent = frame * 100.0 / (float)(startframetrack + numframes);
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    memset(pDstBuf, 0, 4 * numchannels);

    if (pDstBuf)
    {
        Allocator::Free(pDstBuf);
    }

    if (val)
    {
        Allocator::Free(val);
    }

    delete pDecoder;
    return 1;
}

int simportSIGN16LIT_INTf(FileHandle* gs, int channels, int startframe, int endframe, SCOMPSTATE* pcompstate[], short* ptracks[])
{
    int chan;
    int frame;
    short val;
    pcompstate = pcompstate;

    for (frame = startframe; frame <= endframe; frame++)
    {
        for (chan = 0; chan < channels; chan++)
        {
            if (!FileIO::Read(gs, &val, 2))
            {
                SIMEXI_setlasterr("Couldn't read 16-bit samples from file.");
                return 0;
            }

            ptracks[chan][frame] = GetI(&val, 2);
        }

        if ((frame & 0xFFF) == 0)
        {
            float percent = frame * 100.0 / endframe;
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    return 1;
}

int simportUNSIGN8_INTf(FileHandle* gs, int channels, int startframe, int endframe, SCOMPSTATE* pcompstate[], short* ptracks[])
{
    int chan;
    int frame;
    char val;
    pcompstate = pcompstate;

    for (frame = startframe; frame <= endframe; frame++)
    {
        for (chan = 0; chan < channels; chan++)
        {
            if (!FileIO::Read(gs, &val, 1))
            {
                SIMEXI_setlasterr("Couldn't read 8-bit samples from file.");
                return 0;
            }

            ptracks[chan][frame] = (val - 128) << 8;
        }

        if ((frame & 0xFFF) == 0)
        {
            float percent = frame * 100.0 / endframe;
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    return 1;
}

int simportEAXA_helper(FileHandle* gs, short* ptracks[], CEAXABLKDec* pDecoder, int chan, int* pcurframe, __int64* plastfilepos, int numframes)
{
    unsigned char xasbbuf[64];
    XA16STATE xas;
    unsigned int rawnumsamples;
    unsigned int i;
    short *ptemp;
    int framestoup;

    while (numframes > 0)
    {
        if (!FileIO::Read(gs, xasbbuf, 1))
        {
            SIMEXI_setlasterr("Couldn't read XA special block.");
            return 0;
        }

        if (xasbbuf[0] == 238)
        {
            FileIO::Seek(gs, *plastfilepos);

            if (!FileIO::Read(gs, xasbbuf, 5))
            {
                SIMEXI_setlasterr("Couldn't read XA special block.");
                return 0;
            }

            *plastfilepos = FileIO::Tell(gs);
            ptemp = (short*)&xasbbuf[1];
            xas.sample1 = GetM(ptemp, 2);
            ptemp = (short*)&xasbbuf[3];
            xas.sample2 = GetM(ptemp, 2);
            pDecoder->SetState(&xas);

            if (numframes >= 28)
            {
                rawnumsamples = 28;
            }
            else
            {
                rawnumsamples = numframes;
            }

            if (!FileIO::Read(gs, &xasbbuf[5], 2 * rawnumsamples))
            {
                SIMEXI_setlasterr("Couldn't read XA special block.");
                return 0;
            }

            *plastfilepos = FileIO::Tell(gs);
            ptemp = (short*)&xasbbuf[5];

            for (i = 0; i < rawnumsamples; i++)
            {
                ptracks[chan][*pcurframe] = GetM(ptemp, 2);
                (*pcurframe)++;
                ptemp++;
            }

            numframes -= 28;
        }
        else
        {
            FileIO::Seek(gs, *plastfilepos);

            if (!FileIO::Read(gs, xasbbuf, 15))
            {
                SIMEXI_setlasterr("Couldn't read XA samples.");
                return 0;
            }

            *plastfilepos = FileIO::Tell(gs);

            if (numframes >= 28)
            {
                pDecoder->Feed(xasbbuf, 56, 28);
                ptemp = &ptracks[chan][*pcurframe];
                pDecoder->Decode(&ptemp, 28);
                *pcurframe += 28;
            }
            else
            {
                framestoup = numframes;
                pDecoder->Feed(xasbbuf, 2 * framestoup, framestoup);
                ptemp = &ptracks[chan][*pcurframe];
                pDecoder->Decode(&ptemp, framestoup);
                *pcurframe += numframes;
            }

            numframes -= 28;
        }
    }

    return 1;
}

int simportEAXA_BLKf(FileHandle* gs, __int64 sampleoffsets[], int channels, int startframe, int endframe, SCOMPSTATE* pcompstate[], short* ptracks[], SSOUND* pss)
{
    int curframe;
    XA16STATE xas;
    int chan;
    CEAXABLKDec* pDecoder;
    int numframes;
    __int64 startingfilepos = 0;
    int retVal;
    char srcbuf[16];
    __int64 lastfilepos;
    short* ptemp;
    int framestoup;

    for (chan = 0; chan < channels; chan++)
    {
        if (sampleoffsets[chan])
        {
            FileIO::Seek(gs, sampleoffsets[chan]);
        }

        SIMEXI_skipiopheader(gs, pss);

        if (startframe <= 0)
        {
            pDecoder = new CEAXABLKDec();
        }
        else
        {
            pDecoder = (CEAXABLKDec*)pcompstate[chan]->pstate;
        }

        numframes = endframe - startframe + 1;
        curframe = startframe;

        if (!pss->readinfo.codecversion || pss->readinfo.codecversion == 1 && (pss->playloc & 0x100) != 0)
        {
            xas.sample1 = pcompstate[chan]->sample1;
            xas.sample2 = pcompstate[chan]->sample2;
            pDecoder->SetState(&xas);

            while (numframes > 0)
            {
                if (!FileIO::Read(gs, srcbuf, 15))
                {
                    SIMEXI_setlasterr("Couldn't read XA samples.");
                    return 0;
                }

                if (numframes >= 28)
                {
                    pDecoder->Feed(srcbuf, 56, 28);
                    ptemp = &ptracks[chan][curframe];
                    pDecoder->Decode(&ptemp, 28);
                    curframe += 28;
                }
                else
                {
                    framestoup = numframes;
                    pDecoder->Feed(srcbuf, 2 * framestoup, framestoup);
                    ptemp = &ptracks[chan][curframe];
                    pDecoder->Decode(&ptemp, framestoup);
                    curframe += numframes;
                }

                numframes -= 28;
            }
        }
        else
        {
            if ((*pcompstate)->isstream)
            {
                FileIO::Seek(gs, sampleoffsets[chan] - 4);
            }

            lastfilepos = FileIO::Tell(gs);

            if (!chan)
            {
                startingfilepos = FileIO::Tell(gs);
            }

            if (pss->sustainend >= 0)
            {
                numframes = pss->sustainstart + 1;
                retVal = simportEAXA_helper(gs, ptracks, pDecoder, chan, &curframe, &lastfilepos, numframes);

                if (retVal <= 0)
                {
                    return retVal;
                }

                FileIO::Seek(gs, pcompstate[chan]->loopoffset + startingfilepos);
                lastfilepos = FileIO::Tell(gs);
                numframes = pss->sustainend - pss->sustainstart + 1;
                retVal = simportEAXA_helper(gs, ptracks, pDecoder, chan, &curframe, &lastfilepos, numframes);

                if (retVal <= 0)
                {
                    return retVal;
                }
            }
            else
            {
                retVal = simportEAXA_helper(gs, ptracks, pDecoder, chan, &curframe, &lastfilepos, numframes);

                if (retVal <= 0)
                {
                    return retVal;
                }
            }
        }

        xas = pDecoder->GetState();
        pcompstate[chan]->sample1 = xas.sample1;
        pcompstate[chan]->sample2 = xas.sample2;

        if (curframe >= pss->length)
        {
            delete pDecoder;
        }
        else
        {
            pcompstate[chan]->pstate = pDecoder;
        }
    }

    return 1;
}

int simportLAYER123f(FileHandle* pgs, __int64 sampleoffsets[], int channels, int startframe, int endframe, SCOMPSTATE* pcompstate[], short* ptracks[], SSOUND* psound)
{
    void* decoderHandle;
    int samplesdecoded;
    DecoderExtended* pDecoder;
    int numframes;
    int ret;
    __int64 fileSize;
    System* pSndSystem;
    DecoderRegistry* pDecoderRegistry;
    short* pbuf[2];
    char* srcbuf;

    samplesdecoded = 0;
    ret = 1;
    fileSize = FileIO::Length(pgs);
    
    if (channels > 2)
    {
        SIMEXI_setlasterr("Error decoding MPEG Layer 3 data. Only 2 channels supported at this time");
        ret = 0;
        goto abort;
    }

    srcbuf = (char *)Allocator::Alloc(fileSize + 4608);

    if (*sampleoffsets)
    {
        FileIO::Seek(pgs, *sampleoffsets);
        fileSize -= *sampleoffsets;
    }

    numframes = endframe - startframe + 1;

    if (numframes <= 0)
    {
        Allocator::Free(srcbuf);
        return ret;
    }

    pSndSystem = System::GetInstance();
    pSndSystem->Lock();
    pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    decoderHandle = pDecoderRegistry->GetDecoderHandle('MP30');
    pDecoder = pDecoderRegistry->DecoderExtendedFactory(decoderHandle, channels, 1, pSndSystem);
    pSndSystem->Unlock();
    FileIO::Read(pgs, srcbuf, fileSize);
    pDecoder->Feed(srcbuf, numframes, Decoder::FEEDTYPE_NEW);
    pbuf[0] = &ptracks[0][startframe];

    if (psound->numchannels == 2)
    {
        pbuf[1] = &ptracks[1][startframe];
    }

    samplesdecoded = pDecoder->Decode(pbuf, numframes);

    if (samplesdecoded < numframes)
    {
        SIMEXI_setlasterr("Error decoding MPEG Layer 123 data.");
        ret = 0;
        goto abort;
    }

    pDecoder->Release();
    Allocator::Free(srcbuf);

abort:
    return ret;
}

int (*simeximportfn[33])(FileHandle*, __int64[], int, int, int, int, int, int, SCOMPSTATE*[], short*[], SSOUND*) = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, simportSIGN24LIT_INTf, 0, 0};

int SIMEXI_importsamples(FileHandle* gs, __int64 sampleoffsets[], int startframedisk, int startframetrack, int numframes, int numchannels, int samplerep, int flags, SCOMPSTATE* pcompstate[], short* ptracks[], SSOUND* pss)
{
    SIMEXI_vectorCODA();
    int endframetrack = startframetrack + numframes - 1;
    int temp;

    if (endframetrack <= 0)
    {
        return 1;
    }

    if (startframedisk)
    {
        if (samplerep != 21 && samplerep != 1 && samplerep != 2)
        {
            SIMEXI_setlasterr("Unsupported sample representation for computing startframedisk.");
            return 0;
        }

        temp = startframedisk * numchannels;
        
        if (samplerep == 1)
        {
            temp *= 2;
        }
        else if (samplerep == 21)
        {
            temp *= 3;
        }

        *sampleoffsets += temp;
    }

    FileIO::Seek(gs, *sampleoffsets);

    if (samplerep == 0)
    {
        return simportSIGN16LIT_INTf(gs, numchannels, startframetrack, endframetrack, pcompstate, ptracks);
    }
    else if (samplerep == 11)
    {
        return simportUNSIGN8_INTf(gs, numchannels, startframetrack, endframetrack, pcompstate, ptracks);
    }
    else if (samplerep == 10)
    {
        return simportEAXA_BLKf(gs, sampleoffsets, numchannels, startframetrack, endframetrack, pcompstate, ptracks, pss);
    }
    else if (samplerep == 16)
    {
        return simportLAYER123f(gs, sampleoffsets, numchannels, startframetrack, endframetrack, pcompstate, ptracks, pss);
    }
    else if (simeximportfn[samplerep])
    {
        return simeximportfn[samplerep](gs, sampleoffsets, startframedisk, startframetrack, numframes, numchannels, samplerep, flags, pcompstate, ptracks, pss);
    }
    else
    {
        SIMEXI_setlasterr("Unsupported sample import format.");
        return 0;
    }
}

int sexpSIGN16LIT_INT(SINSTANCE* pgi, SSTREAM* pss, int startframe, int endframe, int trackoffsets[], SCOMPSTATE* pcompstate[], int flags, SSOUND* psound)
{
    int frame;
    int chan;
    short val;
    trackoffsets = trackoffsets;
    pcompstate = pcompstate;
    flags = flags;

    for (frame = startframe; frame <= endframe; frame++)
    {
        for (chan = 0; chan < psound->numchannels; chan++)
        {
            PutI(&val, psound->track[chan][frame], 2);
            SIMEXI_streamwrite(pss, &val, 2);
        }

        if ((frame & 0xFFFF) == 0)
        {
            float percent = frame * 100.0 / psound->length;
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    return -1;
}

int writeEAXA(SSTREAM* pss, int startframe, int numsamples, int channel, SSOUND* psound, SCOMPSTATE* pcompstate)
{
    int j;
    int maxframes;
    int frame;
    unsigned char sbheader;
    int trackoffsetinc;
    XA16STATE xas;
    unsigned char xaoutbuf[61];
    int k;
    unsigned char xablock[16];
    CEAXABLKDec* pDecoder;
    int raw_before_crossfade;
    short xasrc[28];
    short* psrc;
    short xatempblock[28];
    int blocknum;
    int write_specialblock;
    short before_encode_sample1;
    short* ptemp;
    short before_encode_sample2;

    trackoffsetinc = 0;
    sbheader = 238;
    maxframes = 28;
    blocknum = 3;
    write_specialblock = 1;
    raw_before_crossfade = 1;
    pDecoder = new CEAXABLKDec();

    if (pcompstate->isstream)
    {
        if (!startframe)
        {
            write_specialblock = 1;
        }
        else
        {
            write_specialblock = 0;
        }
    }

    for (frame = startframe; frame < startframe + numsamples; frame += 28)
    {
        maxframes = 28;

        if (frame + 27 > startframe + numsamples - 28)
        {
            if (startframe + numsamples - frame < 28)
            {
                maxframes = startframe + numsamples - frame;
            }
            
            for (j = 0; j < maxframes; j++)
            {
                xasrc[j] = psound->track[channel][frame + j];
            }

            if (!pcompstate->isstream || (pcompstate->isstream && frame + 27 > psound->length))
            {
                write_specialblock = 1;
                raw_before_crossfade = 0;
                blocknum = 1;
            }

            psrc = xasrc;
        }
        else
        {
            psrc = &psound->track[channel][frame];
        }

        before_encode_sample1 = pcompstate->sample1;
        before_encode_sample2 = pcompstate->sample2;
        sencodexa(psrc, xablock, maxframes, &pcompstate->sample1, &pcompstate->sample2, &pcompstate->diff1, &pcompstate->diff2, 15);
        xas.sample1 = before_encode_sample1;
        xas.sample2 = before_encode_sample2;
        pDecoder->SetState(&xas);
        pDecoder->Feed(xablock, 2 * maxframes, maxframes);
        ptemp = xatempblock;
        pDecoder->Decode(&ptemp, maxframes);

        if (write_specialblock)
        {
            if (raw_before_crossfade)
            {
                if (blocknum == 1)
                {
                    write_specialblock = 0;

                    for (j = 0; j < 28; j++)
                    {
                        xasrc[j] = (psound->track[channel][frame + j] * (28.0 - j) / 28.0);
                        xatempblock[j] = ((float)xatempblock[j] * j / 28.0);
                        xasrc[j] = xasrc[j] + xatempblock[j];
                    }
                }
                else
                {
                    for (j = 0; j < 28; j++)
                    {
                        xasrc[j] = psound->track[channel][frame + j];
                    }
                }
            }
            else
            {
                for (j = 0; j < maxframes; j++)
                {
                    xasrc[j] = ((float)xatempblock[j] * ((float)maxframes - j) / maxframes);
                    xatempblock[j] = ((float)psound->track[channel][frame + j] * j / maxframes);
                    xasrc[j] = xasrc[j] + xatempblock[j];
                }

                k = j;

                for (; j < 28; j++)
                {
                    xasrc[j] = psound->track[channel][frame + k];
                }

                write_specialblock = 0;
            }

            blocknum--;

            PutM(xaoutbuf, sbheader, 1);
            ptemp = (short*)&xaoutbuf[1];
            PutM(ptemp, pcompstate->sample1, 2);
            ptemp = (short*)&xaoutbuf[3];
            PutM(ptemp, pcompstate->sample2, 2);
            ptemp = (short*)&xaoutbuf[5];

            for (j = 0; j < 28; j++)
            {
                PutM(ptemp, xasrc[j], 2);
                ptemp++;
            }

            SIMEXI_streamwrite(pss, xaoutbuf, 61);
            trackoffsetinc += 61;
        }
        else
        {
            SIMEXI_streamwrite(pss, xablock, 15);
            trackoffsetinc += 15;
        }

        if (pcompstate->isstream)
        {
            if (!channel)
            {
                float percent = frame * 100.0 / psound->length;
                SIMEXI_progresscb(FToI::Fast(percent));
            }
        }
        else
        {
            float percent = (frame + channel * psound->length) * 100.0 / (float)(psound->length * psound->numchannels);
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    delete pDecoder;
    return trackoffsetinc;
}

int sexpEAXA_BLK(SINSTANCE* pgi, SSTREAM* pss, int startframe, int endframe, int trackoffsets[], SCOMPSTATE* pcompstate[], int flags, SSOUND* psound)
{
    int byteswritten;
    int trackoffset;
    unsigned char pad[4] = { 0 };
    unsigned char xachar;
    bool writepadding;
    int i;

    trackoffset = 0;
    writepadding = 0;
    byteswritten = 0;

    if (pgi)
    {
        switch (pgi->fileformat)
        {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
        case 9:
        case 20:
        case 22:
        case 24:
        case 30:
        case 32:
        case 34:
        case 35:
        case 38:
        case 40:
        case 49:
            writepadding = 1;
            break;
        case 7:
        case 19:
        case 21:
        case 23:
        case 36:
        case 37:
        case 48:
            writepadding = 0;
            break;
        default:
            writepadding = 0;
            break;
        }

        if ((psound->playloc & 0x100) != 0)
        {
            writepadding = 0;
        }
    }

    for (i = 0; i < psound->numchannels; i++)
    {
        trackoffsets[i] = trackoffset;

        if (writepadding)
        {
            SIMEXI_streamwrite(pss, pad, 4);
            trackoffsets[i] += 4;
            trackoffset += 4;
        }

        byteswritten = 0;

        if (psound->sustainend >= 0)
        {
            if (psound->sustainstart > 0)
            {
                trackoffset += writeEAXA(pss, 0, psound->sustainstart, i, psound, pcompstate[i]);
            }

            pcompstate[i]->loopoffset = trackoffset - 4;
            trackoffset += writeEAXA(pss, psound->sustainstart, psound->sustainend - psound->sustainstart + 1, i, psound, pcompstate[i]);
        }
        else
        {
            trackoffset += writeEAXA(pss, startframe, endframe - startframe + 1, i, psound, pcompstate[i]);
        }

        if ((trackoffset & 1) != 0 && (flags & 0x40) != 0)
        {
            xachar = 0;
            SIMEXI_streamwrite(pss, &xachar, 1);
            trackoffset++;
        }
    }

    return trackoffset;
}

int sexpUNSIGN8_INT(SINSTANCE* pgi, SSTREAM* pss, int startframe, int endframe, int trackoffsets[], SCOMPSTATE* pcompstate[], int flags, SSOUND* psound)
{
    int frame;
    int chan;
    unsigned char val;
    trackoffsets = trackoffsets;
    pcompstate = pcompstate;
    flags = flags;

    for (frame = startframe; frame <= endframe; frame++)
    {
        for (chan = 0; chan < psound->numchannels; chan++)
        {
            val = (psound->track[chan][frame] >> 8) + 0x80;
            SIMEXI_streamwrite(pss, &val, 1);
        }

        if ((frame & 0xFFFF) == 0)
        {
            float percent = frame * 100.0 / psound->length;
            SIMEXI_progresscb(FToI::Fast(percent));
        }
    }

    return -1;
}

int(*simexexportfn[33])(SINSTANCE*, SSTREAM*, int, int, int[], SCOMPSTATE*[], int, SSOUND*) = {sexpSIGN16LIT_INT, 0, 0, 0, 0, 0, 0, 0, 0, 0, sexpEAXA_BLK, sexpUNSIGN8_INT, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int isexportsamples(SINSTANCE* pgi, FileHandle* pFileHandle, void* pmem, int startframe, int numframes, int flags, SCOMPSTATE* pcompstate[], int trackoffsets[], SSOUND* psound)
{
    int byteswritten = -1;
    SSTREAM ss;
    int endframe;

    SIMEXI_vectorCODA();

    if (!numframes)
    {
        return 1;
    }

    if (psound->samplerep < 0 || psound->samplerep >= 33)
    {
        SIMEXI_setlasterr("Unsupported samplerep in export samples.");
        return 0;
    }

    ss.pFileHandle = pFileHandle;
    ss.pmem = (unsigned char*)pmem;
    endframe = startframe + numframes - 1;

    if (simexexportfn[psound->samplerep])
    {
        byteswritten = simexexportfn[psound->samplerep](pgi, &ss, startframe, endframe, trackoffsets, pcompstate, flags, psound);
    }
    else
    {
        SIMEXI_setlasterr("Unsupported sample format in isexportsamples.");
        return 0;
    }

    return byteswritten;
}

int SIMEXI_exportsamplesfile(SINSTANCE* pgi, FileHandle* gs, int startframe, int numframes, int flags, SCOMPSTATE* pcompstate[], int trackoffsets[], SSOUND* psound)
{
    int byteswritten = -1;
    byteswritten = isexportsamples(pgi, gs, 0, startframe, numframes, flags, pcompstate, trackoffsets, psound);

    if (byteswritten == -1)
    {
        return 1;
    }
    else
    {
        return byteswritten;
    }
}
