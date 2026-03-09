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

#ifndef SIMEX_H
#define SIMEX_H

struct FileHandle;

struct SREADRAMINFO
{
    int spusize;
    int iopsize;
    int gcdspsize;
    int ds2or3dsize;
    int maincpusize;
    int padto8bytes;
};

struct SINSTANCE
{
    int numelements;
    int fileformat;
    int uniqueid;
    int ver;
    FileHandle* pFileHandle;
    FileHandle* pFileHandle2;
    char fileName[1024];
    long long fileOffset;
    void* pmem;
    SREADRAMINFO ramuseinfo;
};

struct SENVELOPE
{
    int duration;
    int targetvol;
};

struct SSCALINGTABLE
{
    char xlate[128];
};

struct SREADINFO
{
    long long sampleoffset;
    int codecversion;
    int padto8bytes;
};

struct MARK
{
    int id;
    int position;
    int length;
    char* string;
};

struct MARKCHUNK
{
    int nummarkers;
    MARK* marks[200];
};

struct IMPRESPONSEINFO
{
    short samplesPerFFTBlock;
    short percentHighFrequencyCut;
};

struct SSOUND
{
    SENVELOPE envelope[8];
    SREADINFO readinfo;
    int azimuth[6];
    short* track[64];
    unsigned char* ptssidebanddata[6];
    void* ppropcodebook[6];
    int propcodebooklen[6];
    void* pproploopstate[6];
    int proploopstatelen[6];
    char velmin;
    char velmax;
    char keymin;
    char keymax;
    short detune;
    short randdetune;
    char pan;
    char randpan;
    char panmult;
    char vol;
    unsigned char platformver;
    char randvol;
    char initialenvelopevol;
    char keybase;
    char bend;
    char bendrange;
    char numenvelopes;
    char releaseenvelope;
    char priority;
    char samplerep;
    char numchannels;
    char truncateloops;
    SSCALINGTABLE* pvoltable;
    SSCALINGTABLE* pbendtable;
    unsigned char vollfolength;
    unsigned char pitchlfolength;
    unsigned char vollforandstart;
    unsigned char pitchlforandstart;
    short pitchlfodepth;
    unsigned short playloc;
    SSCALINGTABLE* pvollfo;
    SSCALINGTABLE* ppitchlfo;
    int bitrate;
    int samplerate;
    int length;
    int sustainstart;
    int sustainend;
    int tssidebandsize;
    float gigaInRamPeriod;
    IMPRESPONSEINFO impResponseInfo;
    void* puserdata[4];
    int userdatasize[4];
    MARKCHUNK* pmarkerchunklist[200];
    int markerchunkcount;
    int loopoffset[6];
    int aiffplaymode;
    int aiffsustainstartmarker;
    int aiffsustainendmarker;
    int seekable;
    unsigned char noChannelReordering;
};

struct SINFO
{
    struct SSOUND* sound[32];
    float cps;
    short randdetune;
    unsigned char numsounds;
    unsigned char iscpsdefault;
};

struct SABOUTIMPEXP
{
    char commonvers[8];
    char platformvers[8];
    char samplereps[16];
};

struct SABOUT
{
    SABOUTIMPEXP imp;
    SABOUTIMPEXP exp;
    unsigned int maxelements;
    unsigned int maxtimbres;
    unsigned int maxchannels;
    char formatword[16];
    char formatname[40];
    unsigned int canimport : 1;
    unsigned int canexport : 1;
    unsigned int auxfilerequired : 1;
    unsigned int pan : 1;
    unsigned int randompan : 1;
    unsigned int panmultiplier : 1;
    unsigned int vol : 1;
    unsigned int randomvol : 1;
    unsigned int voltable : 1;
    unsigned int vollfo : 1;
    unsigned int bend : 1;
    unsigned int bendrange : 1;
    unsigned int bendtable : 1;
    unsigned int detune : 1;
    unsigned int randomdetune : 1;
    unsigned int masterrandomdetune : 1;
    unsigned int pitchlfo : 1;
    unsigned int basekey : 1;
    unsigned int priority : 1;
    unsigned int envelope : 1;
    unsigned int loop : 1;
    unsigned int cps : 1;
    unsigned int cbr : 1;
    unsigned int vbr : 1;
    unsigned int playlocdefault : 1;
    unsigned int playlocmaincpu : 1;
    unsigned int playlocspu : 1;
    unsigned int playlociopcpu : 1;
    unsigned int playlocds2dhw : 1;
    unsigned int playlocds3dhw : 1;
    unsigned int playlocdsp : 1;
    unsigned int playlocram : 1;
    unsigned int playlocstream : 1;
    unsigned int playlocgigasample : 1;
    unsigned int azimuth : 1;
    unsigned int padbits : 29;
    int padint;
};

int SIMEX_about(int fileFormat, SABOUT* pSAbout);
int SIMEX_samplerepexpsupported(int fileFormat, int sampleRep);
int SIMEX_id(const char* pFileName, __int64 fileOffset);
int SIMEX_open(const char* pFileName, __int64 fileOffset, int fileFormat, SINSTANCE** ppSInstance);
int SIMEX_info(SINSTANCE* pSInstance, SINFO** ppSInfo, int element);
int SIMEX_read(SINSTANCE* pSInstance, SINFO* pSInfo, int element);
int SIMEX_close(SINSTANCE* pSInstance);
int SIMEX_create(const char* pFileName, int fileFormat, SINSTANCE** ppSInstance);
int SIMEX_write(SINSTANCE* psi, SINFO* psinfo, int element);
int SIMEX_wclose(SINSTANCE* pSInstance);
void SIMEX_defaultssound(SSOUND* psound);
void SIMEX_defaultsinfo(SINFO* psinfo);
int SIMEX_copysinfo(SINFO* pdstsinfo, SINFO* psrcsinfo);
int SIMEX_freessound(SINFO* psinfo, int index);
int SIMEX_freesinfo(SINFO* psinfo);
char *SIMEX_getlasterr();
const char *SIMEX_getsamplerepswitch(int samplerep);

struct SIMEXFILTERPARAMDESC
{
    const char* name;
    const char* cmdline;
    const char* help;
    double minval;
    double maxval;
    const char** stringlist;
    int valtype;
};

struct SIMEXFILTERABOUT
{
    const char* name;
    const char* cmdline;
    const char* help;
    int numparams;
    unsigned char beforeImport;
    unsigned char afterImport;
    SIMEXFILTERPARAMDESC* params;
};

struct SIMEXFILTERPARAM
{
    int intval;
    double doubleval;
    char stringval[28];
    void* pdata;
    int datasize;
};

SIMEXFILTERABOUT* SIMEX_filterabout(int filtertype);
int SIMEX_filterssound(SSOUND* pss, int filtertype, SIMEXFILTERPARAM* psfp);

#endif
