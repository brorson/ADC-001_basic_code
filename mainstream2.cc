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
//  Last Modified : <201006.1144>
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

static int interupted_ = 0;

static void interuptHandler(int sig)
{
    interupted_++;
}

class ForeverStream {
public:
    ForeverStream()
    {
    }
    void start(ADC_Stream *stream,FILE *ofp) {

        ofp_ = ofp;
        stream->Start(ForeverStream::callback__,(void *)this);
    }
private:
    static void callback__(ADC_Stream *stream,uint32_t available,
                           void *usercontext)
    {
        ForeverStream *thispointer = (ForeverStream *) usercontext;
        thispointer->callback_(stream,available);
    }
    void callback_(ADC_Stream *stream,uint32_t available)
    {
        static float buffer[2048];
        uint32_t count, index;
        
        while ((count = stream->GetData(0,2048,buffer,true)) > 0 &&
               interupted_ < 1) {
            for (index = 0; index < count && interupted_ < 1; index++) {
                fprintf(ofp_,"%10.5g\n",buffer[index]);
            }
        }
        if (interupted_ > 0) {
            stream->Stop();
        }
    }
    FILE *ofp_;
    timer_t timerid_;
};

static ADC_Stream adc_stream;
static ForeverStream foreverStream;

int main(int argc, char *argv[])
{
    int seconds = 0;             // Seconds to collect for
    const char *filename;        // Name of file to store values
    
    FILE *ofp;                   // File output stream
    
    
    // Check for CLI args
    if (argc < 2) {
        filename = "-";
    } else {
        filename = argv[1];          // Get the filename
    }
    
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
    
    signal(SIGINT,interuptHandler);
    
    foreverStream.start(&adc_stream,ofp);

    if (strcmp(filename,"-") != 0) fclose(ofp);
    
    exit(EXIT_SUCCESS);
}

