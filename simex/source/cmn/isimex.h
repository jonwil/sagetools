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

#ifndef ISIMEX_H
#define ISIMEX_H

#include <stdint.h>
#include "simex\simex.h"
#include "cmn\mp3toea.h"

struct UTPROFILE
{
    int max_code;
    int voicing_threshold;
    float step_min;
    float step_gain;
    float voiced_scale;
    float unvoiced_scale;
    float anti_imaging;
};

struct UTENCODESTATE
{
    char* outbuffer;
    int bytecount;
    unsigned int shiftreg;
    int bitcount;
    int mode;
    UTPROFILE profile;
    float coeff[12];
    float output[12];
    float history[324];
};

struct UTALKSTATE
{
    unsigned char* dataptr;
    unsigned int shiftreg;
    int bitcount;
    int mode;
    int voicing_threshold;
    float stepval[64];
    float coeff[12];
    float output[12];
    float history[324];
    float data[432];
};

struct MPEGAUDIOHDR
{
    unsigned int bitrate;
    unsigned short samplerate;
    unsigned short numframes;
    unsigned short framebytes;
    unsigned short padshort;
    unsigned char version;
    unsigned char layer;
    unsigned char channels;
    unsigned char mode;
    unsigned char modeext;
    unsigned char crc;
    unsigned char padded;
    unsigned char samplerateindex;
    unsigned char bitrateindex;
    char padchar[3];
};

class CMpegL123DecS16;
class CEALayer3DecS16;

struct LAYER3STATE
{
    MPEGAUDIOHDR mah;
    unsigned int phdr;
    unsigned char* pmp3buffer;
    unsigned int mp3bufsize;
    unsigned int bufferoffset;
    unsigned int chunksize;
    unsigned int bytesencoded;
    CMpegL123DecS16* dec;
    unsigned int num_encoder_passes;
};

struct EALAYER3STATE
{
    unsigned int chunksize;
    unsigned int frameslastdecoded;
    CEALayer3DecS16* pEALayer3Dec;
};

struct SCOMPSTATE
{
    int sample1;
    int sample2;
    double diff1;
    double diff2;
    int stepindex1;
    int sustainstart;
    int sustainend;
    int isstream;
    int samplerate;
    int readoffset;
    int bufframes;
    int platformver;
    unsigned char buf[4096];
    char* lastdataptr;
    UTENCODESTATE utstate;
    UTALKSTATE utdstate;
    void* pUTDecoder;
    LAYER3STATE mp3;
    MP3toEA converter;
    EALAYER3STATE ealayer3;
    unsigned int prevstartframe;
    unsigned int chunksize;
    void* pstate;
    int loopoffset;
};

struct SFILEDRIVER
{
    int  (*about)(struct SABOUT*);
    int  (*is)(const char*, long long, struct FileHandle*);
    int  (*open)(struct SINSTANCE*);
    int  (*info)(struct SINSTANCE*, struct SINFO**, int);
    int  (*read)(struct SINSTANCE*, struct SINFO*, int);
    int  (*close)(struct SINSTANCE*);
    int  (*wopen)(struct SINSTANCE*);
    int  (*write)(struct SINSTANCE*, struct SINFO*, int);
    int  (*wclose)(struct SINSTANCE*);
    int driverEnabled;
}; 

int writesndstreamtimbre(FileHandle* pgs, SINFO* psinfo);
int aboutsndstream(SABOUT* pSAbout);
int issndstream(const char* pFileName, __int64 fileOffset, FileHandle* pgs);
int infosndstream(SINSTANCE* psi, SINFO** ppsinfo, int element);
int readsndstream(SINSTANCE* psi, SINFO* psinfo, int element);
int writesndstream(SINSTANCE* psi, SINFO* psinfo, int element);
int AboutSndPlayer(SABOUT* pSAbout);
int ImportIsSndPlayer(const char* pFileName, __int64 fileOffset, FileHandle* pgs);
int ImportInfoSndPlayer(SINSTANCE* psi, SINFO** ppsinfo, int element);
int ImportReadSndPlayer(SINSTANCE* psi, SINFO* psinfo, int element);
int ExportOpenSndPlayer(SINSTANCE* pSInstance);
int ExportWriteSndPlayer(SINSTANCE* pSInstance, SINFO* pSinfo, int element);

int SIMEXI_rendermodesupported(int fileFormat, SSOUND* pSSound);
int SIMEXI_allocatetracks(SSOUND* psound);
void SIMEXI_resetcompstate(SCOMPSTATE* pstate);
int SIMEXI_allocatecompstate(SCOMPSTATE* pcompstate[], int numTracks);
void SIMEXI_freecompstate(SCOMPSTATE* pcompstate[], int numTracks);
void SIMEXI_progresscb(int percentdone);
void SIMEXI_warningcb(const char* format, ...);
void SIMEXI_setlasterr(const char* str);
void SIMEXI_filterregisterresample();
extern int(*simexfilter[300])(SSOUND*, SIMEXFILTERPARAM*);
extern SIMEXFILTERABOUT simexfilterabout[300];
extern const char* simexfiltertypestrings[300];
int sencodexa(short* psrc, unsigned char* pdst, int frames, int* s1, int* s2, double* d1, double* d2, int allowedfilters);
int SIMEXI_addssound(SINFO* psinfo, int index);
int PlatformToCodecVer(int platform, unsigned char platformver, signed char samplerep);
int puttagmv(FileHandle* pFileHandle, unsigned char tag, int defaultval, int value);
int puttagm(FileHandle* pFileHandle, unsigned char tag, int defaultval, int value, int sizeofval);
int puttagdata(FileHandle* pFileHandle, unsigned char tag, void* pdata, int datasize);
int putmarker(FileHandle* pFileHandle, unsigned char marker);
int SIMEXI_mpegparseheader(unsigned int hdr, MPEGAUDIOHDR* pmah);
int SIMEXI_importsamples(FileHandle* gs, __int64 sampleoffsets[], int startframedisk, int startframetrack, int numframes, int numchannels, int samplerep, int flags, SCOMPSTATE* pcompstate[], short* ptracks[], SSOUND* pss);
void SIMEXI_aligntag(FileHandle* pgs, int datasize, int alignment);
int SIMEXI_exportsamplesfile(SINSTANCE* pgi, FileHandle* gs, int startframe, int numframes, int flags, SCOMPSTATE* pcompstate[], int trackoffsets[], SSOUND* psound);
int aboutwave(SABOUT* pSAbout);
int iswave(const char* pFileName, __int64 fileOffset, FileHandle* pFileHandle);
int infowave(SINSTANCE* psi, SINFO** ppsinfo, int element);
int readwave(SINSTANCE* psi, SINFO* psinfo, int element);
int writewave(SINSTANCE* pSInstance, SINFO* psinfo, int element);
int aboutmpeg(SABOUT* pSAbout);
int ismpeg(const char* pFileName, __int64 fileOffset, FileHandle* pgs);
int infompeg(SINSTANCE* psi, SINFO** ppsinfo, int element);
int readmpeg(SINSTANCE* psi, SINFO* psinfo, int element);

#endif
