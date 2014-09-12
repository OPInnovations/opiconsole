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

/*
  this program generates an EDFplus or BDFplus testfile with the following signals:

 signal label/waveform  amplitude    f       sf
 ---------------------------------------------------
    1    squarewave        100 uV    0.1Hz   200 Hz
    2    ramp              100 uV    1 Hz    200 Hz
    3    pulse 1           100 uV    1 Hz    200 Hz
    4    pulse 2           100 uV    1 Hz    256 Hz
    5    pulse 3           100 uV    1 Hz    217 Hz
    6    noise             100 uV    - Hz    200 Hz
    7    sine 1 Hz         100 uV    1 Hz    200 Hz
    8    sine 8 Hz         100 uV    8 Hz    200 Hz
    9    sine 8.1777 Hz    100 uV    8.25 Hz 200 Hz
   10    sine 8.5 Hz       100 uV    8.5Hz   200 Hz
   11    sine 15 Hz        100 uV   15 Hz    200 Hz
   12    sine 17 Hz        100 uV   17 Hz    200 Hz
   13    sine 50 Hz        100 uV   50 Hz    200 Hz
   14    DC event 8-bits code 1 V   100 mS/bit  200 Hz

*/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "edflib.h"



#define SMP_FREQ 200

#define SMP_FREQ_2 256

#define SMP_FREQ_3 217

#define FILE_DURATION 600


/* if you want to create an EDFplus file instead of BDFplus, outcomment the next line: */


#define BDF_FORMAT




int main(int argc, char *argv[])
{
  int i, j,
      hdl,
      chns;

  double buf[1000],
         q,
         sine_1,
         sine_8,
         sine_81777,
         sine_85,
         sine_15,
         sine_17,
         sine_50;

  struct{
          long long samples;
          long long triggers[512];
          int index;
          int code;
          int bitposition;
          int smp_in_bit;
        } dc_event_stat;



  memset(&dc_event_stat, 0, sizeof(dc_event_stat));

  dc_event_stat.code = 0;

  dc_event_stat.triggers[0] = 1951;

  for(i=1; i<512; i++)
  {
    dc_event_stat.triggers[i] = (i * 1667) + 1951;
  }

  chns = 14;

#ifdef BDF_FORMAT
  hdl = edfopen_file_writeonly("test_generator.bdf", EDFLIB_FILETYPE_BDFPLUS, chns);
#else
  hdl = edfopen_file_writeonly("test_generator.edf", EDFLIB_FILETYPE_EDFPLUS, chns);
#endif

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

  if(edf_set_samplefrequency(hdl, 3, SMP_FREQ_2))
  {
    printf("error: edf_set_samplefrequency()\n");

    return(1);
  }

  if(edf_set_samplefrequency(hdl, 4, SMP_FREQ_3))
  {
    printf("error: edf_set_samplefrequency()\n");

    return(1);
  }

  if(edf_set_samplefrequency(hdl, 13, 1000))
  {
    printf("error: edf_set_samplefrequency()\n");

    return(1);
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

  if(edf_set_digital_minimum(hdl, 13, 8300000))
  {
    printf("error: edf_set_digital_minimum()\n");

    return(1);
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
    if(edf_set_physical_maximum(hdl, i, 1000.0))
    {
      printf("error: edf_set_physical_maximum()\n");

      return(1);
    }
  }

  if(edf_set_physical_maximum(hdl, 8, 262143.0))
  {
    printf("error: edf_set_physical_maximum()\n");

    return(1);
  }

  if(edf_set_physical_maximum(hdl, 13, 10.0))
  {
    printf("error: edf_set_physical_maximum()\n");

    return(1);
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_minimum(hdl, i, -1000.0))
    {
      printf("error: edf_set_physical_minimum()\n");

      return(1);
    }
  }

  if(edf_set_physical_minimum(hdl, 8, -262144.0))
  {
    printf("error: edf_set_physical_minimum()\n");

    return(1);
  }

  if(edf_set_physical_minimum(hdl, 13, -10.0))
  {
    printf("error: edf_set_physical_minimum()\n");

    return(1);
  }

  for(i=0; i<chns; i++)
  {
    if(edf_set_physical_dimension(hdl, i, "uV"))
    {
      printf("error: edf_set_physical_dimension()\n");

      return(1);
    }
  }

  if(edf_set_physical_dimension(hdl, 13, "V"))
  {
    printf("error: edf_set_physical_dimension()\n");

    return(1);
  }

  i = 0;

  if(edf_set_label(hdl, i++, "squarewave"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "ramp"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "pulse 1"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "pulse 2"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "pulse 3"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "noise"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 1 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 8 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 8.1777 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 8.5 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 15 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 17 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "sine 50 Hz"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_label(hdl, i++, "DC 01"))
  {
    printf("error: edf_set_label()\n");

    return(1);
  }

  if(edf_set_equipment(hdl, "test generator"))
  {
    printf("edf_set_equipment()\n");

    return(1);
  }

  edf_set_birthdate(hdl, 1969, 6, 30);


  sine_1 = 0.0;
  sine_8 = 0.0;
  sine_81777 = 0.0;
  sine_85 = 0.0;
  sine_15 = 0.0;
  sine_17 = 0.0;
  sine_50 = 0.0;

  for(j=0; j<FILE_DURATION; j++)
  {
    if((j%10)<5)                    /* square */
    {
      for(i=0; i<SMP_FREQ; i++)
      {
        buf[i] = 100.0;
      }
    }
    else
    {
      for(i=0; i<SMP_FREQ; i++)
      {
        buf[i] = -100.0;
      }
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                       /* ramp */
    {
      buf[i] = -100.0 + (i * (200.0 / SMP_FREQ));
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                 /* pulse 1 */
    {
      buf[i] = 0.0;
    }

    buf[0] = 100.0;

    buf[SMP_FREQ - 2] = 100.0;

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ_2; i++)               /* pulse 2 */
    {
      buf[i] = 0.0;
    }

    buf[0] = 100.0;

    buf[SMP_FREQ_2 - 2] = 100.0;

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ_3; i++)              /* pulse 3 */
    {
      buf[i] = 0.0;
    }

    buf[0] = 100.0;

    buf[SMP_FREQ_3 - 2] = 100.0;

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* noise */
    {
      buf[i] = (int)(100.0 * (rand() / (RAND_MAX + 1.0)));
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 1 Hz */
    {
      q = M_PI * 2.0;
      q /= SMP_FREQ;
      sine_1 += q;
      q = sin(sine_1);
      q *= 100.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 8 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 8.0);
      sine_8 += q;
      q = sin(sine_8);
      q *= 100.0;
      buf[i] = q + 800.0;         /* add dc-offset */
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 8.1777 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 8.1777);
      sine_81777 += q;
      q = sin(sine_81777);
      q *= 100.0;
      buf[i] = q + 6000.0;       /* add dc-offset */
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 8.5 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 8.5);
      sine_85 += q;
      q = sin(sine_85);
      q *= 100.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 15 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 15.0);
      sine_15 += q;
      q = sin(sine_15);
      q *= 100.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 17 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 17.0);
      sine_17 += q;
      q = sin(sine_17);
      q *= 100.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<SMP_FREQ; i++)                /* sine 50 Hz */
    {
      q = M_PI * 2.0;
      q /= (SMP_FREQ / 50.0);
      sine_50 += q;
      q = sin(sine_50);
      q *= 100.0;
      buf[i] = q;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }

    for(i=0; i<1000; i++)                /* DC 01 */
    {
      if(dc_event_stat.bitposition)
      {
        if(dc_event_stat.bitposition == 1)
        {
          buf[i] = 1.0;
        }
        else
        {
          if(dc_event_stat.code & (1 << (dc_event_stat.bitposition - 2)))
          {
            buf[i] = 1.0;
          }
          else
          {
            buf[i] = 0.0;
          }
        }

        if(++dc_event_stat.smp_in_bit >= 10)
        {
          dc_event_stat.smp_in_bit = 0;

          dc_event_stat.bitposition++;
        }

        if(dc_event_stat.bitposition > 10)
        {
          dc_event_stat.bitposition = 0;

          dc_event_stat.smp_in_bit = 0;

          dc_event_stat.code++;

          dc_event_stat.code &= 255;

          if(++dc_event_stat.index >= 512)
          {
            dc_event_stat.index = 0;

            dc_event_stat.code = 0;
          }
        }
      }
      else
      {
        if(dc_event_stat.samples == dc_event_stat.triggers[dc_event_stat.index])
        {
/*          edfwrite_annotation_latin1(hdl, dc_event_stat.samples * 10LL, -1LL, "Trigger");  */

          dc_event_stat.bitposition = 1;
          dc_event_stat.smp_in_bit = 1;
          buf[i] = 1.0;
        }
        else
        {
          buf[i] = 0.0;
        }
      }

      dc_event_stat.samples++;
    }

    if(edfwrite_physical_samples(hdl, buf))
    {
      printf("error: edfwrite_physical_samples()\n");

      return(1);
    }
  }

  edfwrite_annotation_latin1(hdl, 0LL, -1LL, "Recording starts");

  edfwrite_annotation_latin1(hdl, 2980000LL, -1LL, "Test 1");

  edfwrite_annotation_latin1(hdl, 2940000LL + (long long)((10000.0 / SMP_FREQ) * (SMP_FREQ - 2)), -1LL, "pulse 1");

  edfwrite_annotation_latin1(hdl, 2950000LL + (long long)((10000.0 / SMP_FREQ_2) * (SMP_FREQ_2 - 2)), -1LL, "pulse 2");

  edfwrite_annotation_latin1(hdl, 2960000LL + (long long)((10000.0 / SMP_FREQ_3) * (SMP_FREQ_3 - 2)), -1LL, "pulse 3");

  edfwrite_annotation_latin1(hdl, FILE_DURATION * 10000LL, -1LL, "Recording ends");

  edfclose_file(hdl);

  return(0);
}




















