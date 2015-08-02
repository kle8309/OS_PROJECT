// ADCT0ATrigger.h
// Runs on LM4F120/TM4C123
// Provide a function that initializes Timer0A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Daniel Valvano
// September 11, 2013

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_STATUS_BUSY 0
#define ADC_STATUS_DONE 1
#define ADC_STATUS_IDLE 2
 
extern volatile uint32_t ADCsamples; // adc sample counter
extern uint32_t ADCsamplesMax;       // max number of adc samples to acquire
extern uint16_t *ADCBufferPointer;     // global pointer for interrupt usage
extern volatile uint32_t ADCvalue;     // adc data
extern volatile int ADCstatus;  // adc job status

// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer0A: enabled
// Mode: 16-bit, down counting
// One-shot or periodic: periodic
// Prescale value: programmable using variable 'prescale' [0:255]
// Interval value: programmable using variable 'period' [0:65535]
// Sample time is busPeriod*(prescale+1)*(period+1)
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// SS3 1st sample source: programmable using variable 'channelNum' [0:11]
// SS3 interrupts: enabled and promoted to controller
// channelNum must be 0-11 (inclusive) corresponding to Ain0 through Ain11


// device driver for the ADC. Sampling rates should vary from 100 to 10000 Hz, and data will be collected
// on any one of the ADC inputs ADC0 to ADC11. You are free
// to use whatever synchronization mode you wish. Real-time sampling by triggering the conversion from a timer and
// interrupting on ADC completion. In this way, there will be no sampling jitter.
// input:
// channelNum       channel number anywhere from A0 to A11
// fs               sampling frequency from 100 to 10000
// buffer           16-bit buffer for ADC sampling storage
// numberOfSamples  length of buffer
// output:
// 1 error
// 0 success
int ADC_Collect(unsigned int channelNum, unsigned int fs, unsigned short buffer[], unsigned int numberOfSamples);

// Return status of ADC
// 1 done
// 0 busy
int ADC_Status(void);

// config gpio mux
int ADC_Pin_Config(unsigned int channelNum);

// config ADC for channelNum
// sequencer 3 with interrupt
void ADC_Init(unsigned int channelNum);

#endif
