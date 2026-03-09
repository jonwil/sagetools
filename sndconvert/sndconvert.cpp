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

#include "simex\simex.h"
#include "system.h"
#include "encoderregistry.h"
#include "decoderregistry.h"
#include <vector>

struct SimexFilter
{
    int filter;
    std::vector<SIMEXFILTERPARAM> params;
};

int main(int argc, char* argv[])
{
    System *pSndSystem = System::CreateInstance();
    pSndSystem->Lock();
    EncoderRegistry *pEncoderRegistry = pSndSystem->GetEncoderRegistry();
    pEncoderRegistry->RegisterAllEncoders();
    DecoderRegistry *pDecoderRegistry = pSndSystem->GetDecoderRegistry();
    pDecoderRegistry->RegisterAllDecoders();
    pSndSystem->Unlock();
    SABOUT pabout[52];
    memset(pabout, 0, sizeof(pabout));
    const char *infile = 0;
    const char *outfile = 0;
    int format = -1;
    int samplerep = -1;
    std::vector<SimexFilter> filters;

    for (int i = 0; i < 52; i++)
    {
        SIMEX_about(i, &pabout[i]);
    }

    for (int i = 0; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            char *arg = &argv[i][1];
            bool done = false;

            if (!_stricmp(arg, "in"))
            {
                infile = argv[i+1];
            }
            else if (!_stricmp(arg, "out"))
            {
                outfile = argv[i+1];
            }
            else
            {
                for (int j = 0; j < 52; j++)
                {
                    if (!_stricmp(arg, pabout[j].formatword))
                    {
                        format = j;
                        done = true;
                        break;
                    }
                }

                if (!done)
                {
                    for (int j = 0; j < 33; j++)
                    {
                        const char *sr = SIMEX_getsamplerepswitch(j);
                        
                        if (sr && !_stricmp(arg, sr))
                        {
                            samplerep = j;
                            done = true;
                            break;
                        }
                    }
                }

                if (!done)
                {
                    for (int j = 0;j < 300; j++)
                    {
                        SIMEXFILTERABOUT *psfa;
                        psfa = SIMEX_filterabout(j);
                        
                        if (psfa && psfa->cmdline && !_stricmp(arg, psfa->cmdline))
                        {
                            SimexFilter sf;
                            sf.filter = j;

                            for (int k = 0; k < psfa->numparams; k++)
                            {
                                SIMEXFILTERPARAM param;
                                
                                switch (psfa->params[k].valtype)
                                {
                                case 0:
                                    param.intval = atoi(argv[i+1+k]);
                                    break;
                                case 1:
                                    strncpy(param.stringval,argv[i+1+k],sizeof(param.stringval));
                                    break;
                                case 2:
                                    {
                                        FILE *f = fopen(argv[i+1+k], "rb");
                                        fseek(f, 0, SEEK_END);
                                        int size = ftell(f);
                                        fseek(f, 0, SEEK_SET);
                                        param.pdata = malloc(size);
                                        fread(param.pdata, 1, size, f);
                                        param.datasize = size;
                                        fclose(f);
                                    }
                                    break;
                                case 3:
                                    param.doubleval = atof(argv[i+1+k]);
                                    break;
                                }

                                sf.params.push_back(param);
                            }

                            filters.push_back(sf);
                            done = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (infile && outfile && samplerep != -1)
    {
        int srcformat = SIMEX_id(infile, 0);
        
        if (format == -1)
        {
            format = srcformat;
        }

        SINSTANCE *srcinstance;
        SIMEX_open(infile, 0, srcformat, &srcinstance);
        SINFO *info;
        SIMEX_info(srcinstance, &info, 0);
        SIMEX_read(srcinstance, info, 0);

        for (unsigned int i = 0; i < filters.size(); i++)
        {
            SIMEX_filterssound(info->sound[0], filters[i].filter, &filters[i].params[0]);
        }

        info->sound[0]->samplerep = samplerep;
        SINSTANCE *dstinstance;
        SIMEX_create(outfile, format, &dstinstance);
        SIMEX_write(dstinstance, info, 0);
        SIMEX_freesinfo(info);
        SIMEX_close(srcinstance);
        SIMEX_wclose(dstinstance);
    }

    pSndSystem->Release();
    return 0;
}
