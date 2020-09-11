// -!- c++ -!- //////////////////////////////////////////////////////////////
//
//  System        : 
//  Module        : 
//  Object Name   : $RCSfile$
//  Revision      : $Revision$
//  Date          : $Date$
//  Author        : $Author$
//  Created By    : Robert Heller
//  Created       : Sat Sep 5 14:26:26 2020
//  Last Modified : <200906.1004>
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

#ifndef __ADC_STREAM_H
#define __ADC_STREAM_H

#include <stdint.h>
#include <pthread.h>

/** Buffer sizes.  These are the sizes of the buffers
 */

#define SPIBUFFERSIZE 1024 // Low-level SPI buffer
#define RINGBUFFERSIZE (8*SPIBUFFERSIZE) // High-level ring buffer


/** ADC_Stream, a class to encapsulate streaming data from the ADC 
 * Cape.  This class implements continious streaming from the ADC,
 * using a ring buffer.  The low-level PRU SPI transfers are handled 
 * with a separate thread, while the main thread processes the data
 * that is available in the ring buffer.
 * 
 */

class ADC_Stream {
public:
    /** Callback function type.  This is the type of the user-supplied
     * call back function.  This function is called as new data 
     * becomes available.  It is passed the class instance's this
     * pointer, the number of 32-bit floating point values currently
     * available, and a user supplied context pointer.
     */
    typedef void (*Callback_t)(ADC_Stream *stream,uint32_t available,
                               void *usercontext);
    /** Constructor: initialize instance variables.
     */
    ADC_Stream()
                : highmutex_(PTHREAD_MUTEX_INITIALIZER)
          , inpointer_(0)
          , outpointer_(0)
          , end_(true)
    {
    }
    /** Start function.  This function starts the data collection
     * thread.
     * The data collection thread is started.  Once a batch of data 
     * becomes available, the callback function is called.  The
     * callback function is called whenever an additional batch of
     * data becomes available.  
     * Parameters:
     *     callback is the callback function.
     *     usercontext is a user supplied pointer that gets
     *                 passed to the callback function.
     * Returns void
     */
    void Start(Callback_t callback, void *usercontext = NULL);
    /** End function.  This function stops the data collection thread.
     * The data collection thread is stopped and data streaming
     * ceases.
     * Parameters:
     *      None.
     * Returns void
     */
    void Stop(void);
    /** GetData function.  This function fetches a block of data from
     * the ring buffer and optionally advances the outpointer.
     * Parameters:
     *     offset is the offset from the outpointer to start copying
     *            data from
     *     count is the maximum number of 32-bit floats to copy.
     *           If fewer 32-bit floats are available, then only
     *           number of available 32-bit floats are copied.
     *     buffer is where to copy the data to.
     *     update is a flag to indicate if the outpointer should be
     *            advanced.  If true, the outpointer is advanced by
     *            offset+count 32-bit floats, that is to just after
     *            the block fetched.
     * Returns the actual number of 32-bit floats copied.
     */
    uint32_t GetData(uint32_t offset, uint32_t count, float *buffer, 
                     bool update=false);
private:
    /** Static thread function.
     * This static function just calls the member function that 
     * implements the thread.
     * Parameters:
     *     param is the this pointer
     * Returns the result of the thread
     */
    static void *thread__(void *param)
    {
        ADC_Stream *thispointer = (ADC_Stream *) param;
        return thispointer->thread_();
    }
    /** Low level buffering thread.  This is the implementation of the
     * low-level buffering thread.
     * Parameters:
     *     None
     * Returns the result of the thread.
     */
    void *thread_();
    /** User callback function.  This is the user callback function.
      */    
    Callback_t callback_;
    /** User context pointer.  This us the user callback context 
     * pointer
     */
    void *     usercontext_;
    /** The ring buffer.  This is the large circular (ring) buffer
     * containing the raw samples from the ADC.
     */
    uint32_t RingBuffer_[RINGBUFFERSIZE];
    /** Input pointer (index) of the ring buffer.  New data is added
     * at this index.
     */
    int inpointer_;
    /** Output pointer (index) of the ring buffer.  Application data
     * is returned starting from this index.
     */
    int outpointer_;
    /** Mutex for the high-level Ring buffer. */
    pthread_mutex_t highmutex_;
    /** End flag. When true, streaming is stopped. */
    bool end_;
    /** Thread object. */
    pthread_t threadHandle_;
    /** New data flag */
    bool newdata_;
};
    

#endif // __ADC_STREAM_H

