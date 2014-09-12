/*
*****************************************************************************
*
* Copyright (c) 2009, 2010, 2011, 2012, 2013 Teunis van Beelen
* All rights reserved.
*
* email: teuniz@gmail.com
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY Teunis van Beelen ''AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Teunis van Beelen BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************
*/



#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "edflib.h"




/* if you want to create an EDFplus file instead of BDFplus, outcomment the next line: */
//#define BDF_FORMAT



void remove_trailing_zeros(char *);



int main(int argc, char *argv[])
{
  int i, j,
      hdl,
      chns=1,
      smp_freq=8192,
      fileduration=300,
      linear=1;

  double buf[smp_freq],
         q,
         sine_1,
         startfreq=10.0,
         stopfreq=1000.0,
         freqspan,
         freq;

  long long samples,
            sampleswritten;

  char str[256];



#ifdef BDF_FORMAT
  hdl = edfopen_file_writeonly("freq_sweep.bdf", EDFLIB_FILETYPE_BDFPLUS, chns);
#else
  hdl = edfopen_file_writeonly("freq_sweep.edf", EDFLIB_FILETYPE_EDFPLUS, chns);
#endif

  if(hdl<0)
  {
    printf("error: edfopen_file_writeonly()\n");

    return(1);
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_samplefrequency(hdl, i, smp_freq))
    {
      printf("error: edf_set_samplefrequency()\n");

      return(1);
    }
  }

#ifdef BDF_FORMAT
  for(i=0; i<chns; i++)
  {
    if(edf_set_digital_maximum(hdl, i, 8388607))
    {
      printf("error: edf_set_digital_maximum()\n");

      return(1);
    }
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_digital_minimum(hdl, i, -8388608))
    {
      printf("error: edf_set_digital_minimum()\n");

      return(1);
    }
  }
#else
  for(i=0; i<chns; i++)
  {
    if(edf_set_digital_maximum(hdl, i, 32767))
    {
      printf("error: edf_set_digital_maximum()\n");

      return(1);
    }
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_digital_minimum(hdl, i, -32768))
    {
      printf("error: edf_set_digital_minimum()\n");

      return(1);
    }
  }
#endif

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_maximum(hdl, i, 400.0))
    {
      printf("error: edf_set_physical_maximum()\n");

      return(1);
    }
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_minimum(hdl, i, -400.0))
    {
      printf("error: edf_set_physical_minimum()\n");

      return(1);
    }
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_dimension(hdl, i, "uV"))
    {
      printf("error: edf_set_physical_dimension()\n");

      return(1);
    }
  }

  if(edf_set_label(hdl, 0, "sweep"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_equipment(hdl, "Software generated file"))
  {
    printf("edf_set_equipment()\n");

    return(1);
  }

  if(linear)
  {
    sprintf(str, "Linear frequency sweep from %fHz to %fHz", startfreq, stopfreq);
  }
  else
  {
    sprintf(str, "Logarithmic frequency sweep from %fHz to %fHz", startfreq, stopfreq);
  }
  remove_trailing_zeros(str);
  edf_set_patientname(hdl, str);

  edfwrite_annotation_latin1(hdl, 0LL, -1LL, "Recording starts");

  sine_1 = 0.0;

  sampleswritten = 0;

  samples = fileduration * (long long)smp_freq;

  freqspan = stopfreq - startfreq;

  for(j=0; j<fileduration; j++)
  {
    for(i=0; i<smp_freq; i++)
    {
      q = M_PI * 2.0;
      q /= (smp_freq / freq);
      sine_1 += q;
      q = sin(sine_1);
      q *= 200.0;
      buf[i] = q;
      if(linear)
      {
        freq = startfreq + (freqspan * ((double)sampleswritten / (double)samples));
      }
      else
      {
//        freq = exp10((((double)sampleswritten / (double)samples)) * log10(stopfreq));
        freq = exp10(((((startfreq / stopfreq) * ((stopfreq / freqspan) * samples)) + sampleswritten) / ((stopfreq / freqspan) * samples)) * log10(stopfreq));
      }
      sampleswritten++;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    if(!(j%10))
    {
      sprintf(str, "%fHz", freq);
      remove_trailing_zeros(str);
      edfwrite_annotation_latin1(hdl, j * 10000LL, -1LL, str);
    }
  }

  sprintf(str, "%fHz", freq);
  remove_trailing_zeros(str);
  edfwrite_annotation_latin1(hdl, j * 10000LL, -1LL, str);

  edfwrite_annotation_latin1(hdl, fileduration * 10000LL, -1LL, "Recording ends");

  edfclose_file(hdl);

  return(0);
}



void remove_trailing_zeros(char *str)
{
  int i, j,
      len,
      numberfound,
      dotfound,
      decimalzerofound,
      trailingzerofound=1;

  while(trailingzerofound)
  {
    numberfound = 0;
    dotfound = 0;
    decimalzerofound = 0;
    trailingzerofound = 0;

    len = strlen(str);

    for(i=0; i<len; i++)
    {
      if((str[i] < '0') || (str[i] > '9'))
      {
        if(decimalzerofound)
        {
          if(str[i-decimalzerofound-1] == '.')
          {
            decimalzerofound++;
          }

          for(j=i; j<(len+1); j++)
          {
            str[j-decimalzerofound] = str[j];
          }

          trailingzerofound = 1;

          break;
        }

        if(str[i] != '.')
        {
          numberfound = 0;
          dotfound = 0;
          decimalzerofound = 0;
        }
      }
      else
      {
        numberfound = 1;

        if(str[i] > '0')
        {
          decimalzerofound = 0;
        }
      }

      if((str[i] == '.') && numberfound)
      {
        dotfound = 1;
      }

      if((str[i] == '0') && dotfound)
      {
        decimalzerofound++;
      }
    }
  }

  if(decimalzerofound)
  {
    if(str[i-decimalzerofound-1] == '.')
    {
      decimalzerofound++;
    }

    for(j=i; j<(len+1); j++)
    {
      str[j-decimalzerofound] = str[j];
    }
  }
}

















