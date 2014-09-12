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





#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "edflib.h"


#define SMP_FREQ 2048




int main(int argc, char *argv[])
{
  int i, j,
      hdl,
      buf2[SMP_FREQ],
      chns;

  double buf[SMP_FREQ],
         q;



  chns = 2;

  hdl = edfopen_file_writeonly("sine.bdf", EDFLIB_FILETYPE_BDFPLUS, chns);

  if(hdl<0)
  {
    printf("error: edfopen_file_writeonly()\n");

    return(1);
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_samplefrequency(hdl, i, SMP_FREQ))
    {
      printf("error: edf_set_samplefrequency()\n");

      return(1);
    }
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_maximum(hdl, i, 3000.0))
    {
      printf("error: edf_set_physical_maximum()\n");

      return(1);
    }
  }

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

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_minimum(hdl, i, -3000.0))
    {
      printf("error: edf_set_physical_minimum()\n");

      return(1);
    }
  }

  if(edf_set_label(hdl, 0, "sine"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, 1, "ramp"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_dimension(hdl, i, "mV"))
    {
      printf("error: edf_set_physical_dimension()\n");

      return(1);
    }
  }

  for(j=0; j<10; j++)
  {
    for(i=0; i<SMP_FREQ; i++)
    {
      q = M_PI * 2.0;
      q /= SMP_FREQ;
      q *= i;
      q = sin(q);
      q *= 3000.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)
    {
      buf[i] = -3000.0 + (i * 2.9296875);
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }
  }

  for(j=0; j<10; j++)
  {
    for(i=0; i<SMP_FREQ; i++)
    {
      q = M_PI * 2.0;
      q /= SMP_FREQ;
      q *= i;
      q = sin(q);
      q *= 8388607.0;
      buf2[i] = q;
    }

    if(edfwrite_digital_samples(hdl, buf2))
    {
      printf("error: edfwrite_digital_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)
    {
      buf2[i] = -8388608 + (i * 8192);
    }

    if(edfwrite_digital_samples(hdl, buf2))
    {
      printf("error: edfwrite_digital_samples()\n");

      return(1);
    }
  }

  edfwrite_annotation_latin1(hdl, 0LL, -1LL, "Recording starts");

  edfwrite_annotation_latin1(hdl, 200000LL, -1LL, "Recording ends");

  edfclose_file(hdl);

  return(0);
}




















