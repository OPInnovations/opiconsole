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

#include "edflib.h"






int main(int argc, char *argv[])
{
  int i,
      hdl,
      channel,
      n;

  double *buf;

  struct edf_hdr_struct hdr;


  if(argc!=3)
  {
    printf("\nusage: test_edflib <file> <signal nr>\n\n");
    return(1);
  }

  channel = atoi(argv[2]);
  if(channel<1)
  {
    printf("\nsignalnumber must be > 0\n\n");
    return(1);
  }

  if(edfopen_file_readonly(argv[1], &hdr, EDFLIB_READ_ALL_ANNOTATIONS))
  {
    switch(hdr.filetype)
    {
      case EDFLIB_MALLOC_ERROR                : printf("\nmalloc error\n\n");
                                                break;
      case EDFLIB_NO_SUCH_FILE_OR_DIRECTORY   : printf("\ncan not open file, no such file or directory\n\n");
                                                break;
      case EDFLIB_FILE_CONTAINS_FORMAT_ERRORS : printf("\nthe file is not EDF(+) or BDF(+) compliant\n"
                                                       "(it contains format errors)\n\n");
                                                break;
      case EDFLIB_MAXFILES_REACHED            : printf("\nto many files opened\n\n");
                                                break;
      case EDFLIB_FILE_READ_ERROR             : printf("\na read error occurred\n\n");
                                                break;
      case EDFLIB_FILE_ALREADY_OPENED         : printf("\nfile has already been opened\n\n");
                                                break;
      default                                 : printf("\nunknown error\n\n");
                                                break;
    }

    return(1);
  }

  hdl = hdr.handle;

  if(channel>(hdr.edfsignals))
  {
    printf("\nerror: file has %i signals and you selected signal %i\n\n", hdr.edfsignals, channel);
    edfclose_file(hdl);
    return(1);
  }

  channel--;

  printf("\nlibrary version: %i.%02i\n", edflib_version() / 100, edflib_version() % 100);

  printf("\ngeneral header:\n\n");

  printf("filetype: %i\n", hdr.filetype);
  printf("edfsignals: %i\n", hdr.edfsignals);
#ifdef WIN32
  printf("file duration: %I64d seconds\n", hdr.file_duration / EDFLIB_TIME_DIMENSION);
#else
  printf("file duration: %lli seconds\n", hdr.file_duration / EDFLIB_TIME_DIMENSION);
#endif
  printf("startdate: %i-%i-%i\n", hdr.startdate_day, hdr.startdate_month, hdr.startdate_year);
  printf("starttime: %i:%02i:%02i\n", hdr.starttime_hour, hdr.starttime_minute, hdr.starttime_second);
  printf("patient: %s\n", hdr.patient);
  printf("recording: %s\n", hdr.recording);
  printf("patientcode: %s\n", hdr.patientcode);
  printf("gender: %s\n", hdr.gender);
  printf("birthdate: %s\n", hdr.birthdate);
  printf("patient_name: %s\n", hdr.patient_name);
  printf("patient_additional: %s\n", hdr.patient_additional);
  printf("admincode: %s\n", hdr.admincode);
  printf("technician: %s\n", hdr.technician);
  printf("equipment: %s\n", hdr.equipment);
  printf("recording_additional: %s\n", hdr.recording_additional);
  printf("datarecord duration: %f seconds\n", ((double)hdr.datarecord_duration) / EDFLIB_TIME_DIMENSION);
#ifdef WIN32
  printf("number of datarecords in the file: %I64d\n", hdr.datarecords_in_file);
  printf("number of annotations in the file: %I64d\n", hdr.annotations_in_file);
#else
  printf("number of datarecords in the file: %lli\n", hdr.datarecords_in_file);
  printf("number of annotations in the file: %lli\n", hdr.annotations_in_file);
#endif

  printf("\nsignal parameters:\n\n");

  printf("label: %s\n", hdr.signalparam[channel].label);
#ifdef WIN32
  printf("samples in file: %I64d\n", hdr.signalparam[channel].smp_in_file);
#else
  printf("samples in file: %lli\n", hdr.signalparam[channel].smp_in_file);
#endif
  printf("samples in datarecord: %i\n", hdr.signalparam[channel].smp_in_datarecord);
  printf("physical maximum: %f\n", hdr.signalparam[channel].phys_max);
  printf("physical minimum: %f\n", hdr.signalparam[channel].phys_min);
  printf("digital maximum: %i\n", hdr.signalparam[channel].dig_max);
  printf("digital minimum: %i\n", hdr.signalparam[channel].dig_min);
  printf("physical dimension: %s\n", hdr.signalparam[channel].physdimension);
  printf("prefilter: %s\n", hdr.signalparam[channel].prefilter);
  printf("transducer: %s\n", hdr.signalparam[channel].transducer);
  printf("samplefrequency: %f\n", ((double)hdr.signalparam[channel].smp_in_datarecord / (double)hdr.datarecord_duration) * EDFLIB_TIME_DIMENSION);

  struct edf_annotation_struct annot;

  printf("\n");

  for(i=0; i<hdr.annotations_in_file; i++)
  {
    if(edf_get_annotation(hdl, i, &annot))
    {
      printf("\nerror: edf_get_annotations()\n");
      edfclose_file(hdl);
      return(1);
    }
    else
    {
#ifdef WIN32
      printf("annotation: onset is %I64d    duration is %s    description is %s\n",
            annot.onset / EDFLIB_TIME_DIMENSION,
            annot.duration,
            annot.annotation);
#else
      printf("annotation: onset is %lli    duration is %s    description is %s\n",
            annot.onset / EDFLIB_TIME_DIMENSION,
            annot.duration,
            annot.annotation);
#endif
    }
  }

  n = 200; /* read n samples from the file */

  buf = (double *)malloc(sizeof(double[n]));
  if(buf==NULL)
  {
    printf("\nmalloc error\n");
    edfclose_file(hdl);
    return(1);
  }

  int x=10; /* start reading x seconds from start of file */

  edfseek(hdl, channel, (long long)((((double)x) / ((double)hdr.file_duration / (double)EDFLIB_TIME_DIMENSION)) * ((double)hdr.signalparam[channel].smp_in_file)), EDFSEEK_SET);

  n = edfread_physical_samples(hdl, channel, n, buf);

  if(n==(-1))
  {
    printf("\nerror: edf_read_physical_samples()\n");
    edfclose_file(hdl);
    free(buf);
    return(1);
  }

  printf("\nread %i samples, started at %i seconds from start of file:\n\n", n, x);

  for(i=0; i<n; i++)
  {
    printf("%.0f  ", buf[i]);
  }

  printf("\n\n");

  edfclose_file(hdl);

  free(buf);

  return(0);
}





























