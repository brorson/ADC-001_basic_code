// -!- C++ -!- //////////////////////////////////////////////////////////////
//
//  System        : 
//  Module        : 
//  Object Name   : $RCSfile$
//  Revision      : $Revision$
//  Date          : $Date$
//  Author        : $Author$
//  Created By    : Robert Heller
//  Created       : Sat Sep 5 15:17:04 2020
//  Last Modified : <200914.1235>
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
#include <stdint.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>

#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#include "spidriver_host.h"
#include "adcdriver_host.h"

#include "ADC_Stream.h"

#define SPI_PRU	0
#define CLK_PRU 1

// Commands to AD7172
#define READ_ID_REG 0x47
#define READ_DATA_REG 0x44
#define READ_STATUS_REG 0x40
#define WRITE_CH0_REG 0x10
#define WRITE_CH1_REG 0x11
#define WRITE_SETUPCON0_REG 0x20
#define WRITE_ADCMODE_REG 0x01
#define WRITE_IFMODE_REG 0x02
#define WRITE_GPIOCON_REG 0x06
#define WRITE_FILTERCON0_REG 0x28

/** Start function implementation.
 */
void ADC_Stream::Start(Callback_t callback, void *usercontext)
{
    // Initialize instance variables:
    callback_ = callback;          // Save Callback function
    usercontext_ = usercontext;    // Save User Context
    end_ = false;                  // Not the end
    newdata_ = false;              // New Data flag (no new data yet).
    
    inpointer_ = 0;                // Reset Ring Buffer pointer indexes
    outpointer_ = 0;
    // Create thread to read data
    int result = pthread_create(&threadHandle_,NULL,
                                ADC_Stream::thread__,
                                (void *)this);
    // Error check
    if (result != 0)
    {
        perror("ADC_Stream::Start:");
        exit(result);
    }
    // Loop until end.
    while (!end_) {
        usleep(100000);             // sleep 100 ms.
        if (newdata_) {             // new data check
            newdata_ = false;       // reset flag
            // Compute how much data is available
            uint32_t dataavailable = inpointer_ - outpointer_;
            // Wrap around
            if (dataavailable < 0) {
                dataavailable = RINGBUFFERSIZE - dataavailable;
            }
            // Call Callback function
            (*callback_)(this,dataavailable,usercontext_);
        }
    }
}

/** Stop function implementation */
void ADC_Stream::Stop(void)
{
    void *result = NULL; // holds result (not used)
    end_ = true;         // Set end flag
    
    pthread_join(threadHandle_,&result); // Wait for thread to stop
}

/** Get data function implementation */
uint32_t ADC_Stream::GetData(uint32_t offset, uint32_t count, 
                             float *buffer, bool update)
{
    int i;
    // Compute data available
    uint32_t dataavailable = inpointer_ - outpointer_;
    // Wraparound
    if (dataavailable < 0) {
        dataavailable = RINGBUFFERSIZE - dataavailable;
    }
    // Adjust count, if data available is less than count.
    if (dataavailable < count+offset) count = dataavailable-offset;
    // Compute start index
    uint32_t start = (outpointer_+offset)%RINGBUFFERSIZE;
    // Check for wraparound
    if (start + count < RINGBUFFERSIZE) {
        // No wraparound, convert to float into user buffer
        for (i = 0; i < count; i++) {
            buffer[i] = adc_GetVoltage(RingBuffer_[start+i]);
        }
    } else {
        // Wraparound case
        // Before 12 o'clock part
        uint32_t count1 = RINGBUFFERSIZE - start;
        for (i = 0; i < count1; i++) {
            buffer[i] = adc_GetVoltage(RingBuffer_[start+i]);
        }
        // After 12 o'clock part 
        buffer+=count1;
        uint32_t remainder = count-count1;
        for (i = 0; i < remainder; i++) {
            buffer[i] = adc_GetVoltage(RingBuffer_[i]);
        }
    }
    if (update) {    // Check if updating pointer
        pthread_mutex_lock(&highmutex_);
        outpointer_ = (outpointer_+offset+count)%RINGBUFFERSIZE;
        pthread_mutex_unlock(&highmutex_);
    }
    return count;
}

/** Data aquistation thread implementation */
void * ADC_Stream::thread_()
{
    // command buffer
    uint32_t tx_buf[3];
    int i, j;
    uint32_t prev_rxptr, curr_rxptr;
    uint32_t current_offset;
    
    // Set up ADC mode reg for continuous conversation
    
    // Write mode register
    //printf("WRITE_ADCMODE_REG\n");
    tx_buf[0] = WRITE_ADCMODE_REG;
    tx_buf[1] = 0x00;
    tx_buf[2] = 0x0c;
    spi_write_cmd(tx_buf, 3);
    
    // Read samples from data register to buffer
    tx_buf[0] = READ_DATA_REG;
    // Initialize by filling the buffer at offset 0.
    prev_rxptr = spi_writeread_continuous_start(tx_buf, 1, 0, 3, 
                                                SPIBUFFERSIZE);
    //fprintf(stderr,"*** ADC_Stream::thread_(): after spi_writeread_continuous_start - prev_rxptr is %d\n",prev_rxptr);
    // Then we will fill the buffer at SPIBUFFERSIZE offset
    current_offset = SPIBUFFERSIZE;
    // Loop until end
    while (!end_) {
        //fprintf(stderr,"*** ADC_Stream::thread_(): top of while loop: current_offset = %d\n",current_offset);
        // Wait for the previous buffer to fill, then start filling
        // the other buffer.
        spi_writeread_continuous_wait();
        
        // Write mode register
        //printf("WRITE_ADCMODE_REG\n");
        tx_buf[0] = WRITE_ADCMODE_REG;
        tx_buf[1] = 0x00;
        tx_buf[2] = 0x0c;
        spi_write_cmd(tx_buf, 3);
        
        // Read samples from data register to buffer
        tx_buf[0] = READ_DATA_REG;
        curr_rxptr = spi_writeread_continuous_start(tx_buf, 1,
                                                        current_offset,
                                                        3, 
                                                        SPIBUFFERSIZE);
        //fprintf(stderr,"*** ADC_Stream::thread_(): after spi_writeread_continuous_wait - prev_rxptr = %d\n",prev_rxptr);
        // Copy the previous buffer to the ring.
        spi_writeread_continuous_transfer(prev_rxptr, SPIBUFFERSIZE,
                                          &RingBuffer_[inpointer_]);
        pthread_mutex_lock(&highmutex_); // Lock
        // Update point index
        inpointer_ = (inpointer_+SPIBUFFERSIZE) % RINGBUFFERSIZE;
        pthread_mutex_unlock(&highmutex_); // unlock
        // Flag newdata
        newdata_ = true;
        // Current buffer becomes the previous buffer
        prev_rxptr = curr_rxptr; 
        // Swap buffer offsets in the PRU
        if (current_offset == SPIBUFFERSIZE) {
            current_offset = 0;
        } else {
            current_offset = SPIBUFFERSIZE;
        }
    }
}

            
