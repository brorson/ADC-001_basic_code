// -!- C++ -!- //////////////////////////////////////////////////////////////
//
//  System        : 
//  Module        : 
//  Object Name   : $RCSfile$
//  Revision      : $Revision$
//  Date          : $Date$
//  Author        : $Author$
//  Created By    : Robert Heller
//  Created       : Fri Sep 11 13:08:51 2020
//  Last Modified : <200911.1429>
//
//  Description	
//
//  Notes
//
//  History
//	
/////////////////////////////////////////////////////////////////////////////
//
//    Copyright (C) 2020  Robert Heller D/B/A Deepwoods Software
//			51 Locke Hill Road
//			Wendell, MA 01379-9728
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// 
//
//////////////////////////////////////////////////////////////////////////////

static const char rcsid[] = "@(#) : $Id$";

#include <stdio.h>
#include <stdlib.h>
// #include <stdarg.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#include "spidriver_host.h"
#include "adcdriver_host.h"

#include "ADC_Stream.h"

class TimedStream {
public:
    TimedStream()
    {
    }
    void start(ADC_Stream *stream,int seconds,FILE *ofp) {

        struct sigevent sev;         // Timer event
        struct itimerspec its;       // Timer interval structs.

        sev.sigev_notify = SIGEV_NONE;
        if (timer_create(CLOCK_REALTIME, &sev, &timerid_) == -1) {
            perror("timer_create");
            exit(EXIT_FAILURE);
        }
        its.it_value.tv_sec = seconds;
        its.it_value.tv_nsec = 0;
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        
        if (timer_settime(timerid_, 0, &its, NULL) == -1) {
            perror("timer_settime");
            exit(EXIT_FAILURE);
        }

        ofp_ = ofp;
        stream->Start(TimedStream::callback__,(void *)this);
    }
private:
    static void callback__(ADC_Stream *stream,uint32_t available,
                           void *usercontext)
    {
        TimedStream *thispointer = (TimedStream *) usercontext;
        thispointer->callback_(stream,available);
    }
    void callback_(ADC_Stream *stream,uint32_t available)
    {
        float buffer[1024];
        uint32_t count, index;
        struct itimerspec current;
        
        while ((count = stream->GetData(0,1024,buffer,true)) > 0) {
            for (index = 0; index < count; index++) {
                fprintf(ofp_,"%10.5g\n",buffer[index]);
            }
            if (timer_gettime(timerid_,&current) == -1) {
                perror("timer_gettime");
                exit(EXIT_FAILURE);
            }
            if (current.it_value.tv_sec == 0 &&
                current.it_value.tv_nsec == 0) {
                stream->Stop();
            }
        }
    }
    FILE *ofp_;
    timer_t timerid_;
};

int main(int argc, char *argv[])
{
    int seconds = 0;             // Seconds to collect for
    const char *filename;        // Name of file to store values
    
    FILE *ofp;                   // File output stream
    
    ADC_Stream adc_stream;
    TimedStream timedStream;
    
    // Check for CLI args
    if (argc < 3) {
        fprintf(stderr,"usage: %s seconds filename\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    seconds = atoi(argv[1]);     // Get the seconds
    filename = argv[2];          // Get the filename
    
    // Initialize A/D converter
    adc_config();
    
    // Set the sample rate to 15kSPS
    adc_set_samplerate(SAMP_RATE_15625); 
    
    // Set to channel 0
    adc_set_chan0();
    
    if (strcmp(filename,"-") == 0) {
        ofp = stdout;                 // "-" means stdout (terminal)
    } else {
        ofp = fopen(filename,"w");
        if (ofp == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }
            
    timedStream.start(&adc_stream,seconds,ofp);

    if (strcmp(filename,"-") != 0) fclose(ofp);
    
    exit(EXIT_SUCCESS);
}

