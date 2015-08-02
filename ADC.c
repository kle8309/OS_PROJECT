// ADC.c
// Runs on LM4F120/TM4C123
// Provide a function that initializes Timer0A to trigger ADC
// SS3 conversions and request an interrupt when the conversion
// is complete.
// Daniel Valvano
// September 11, 2013
// Modified by
// Kelvin Le
// January 31, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
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
#include <stdint.h>
#include "adc.h"
#include "Timer0.h"
#include "tm4c123gh6pm.h"
#include "debug.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// There are many choices to make when using the ADC, and many
// different combinations of settings will all do basically the
// same thing.  For simplicity, this function makes some choices
// for you.  When calling this function, be sure that it does
// not conflict with any other software that may be running on
// the microcontroller.  Particularly, ADC0 sample sequencer 3
// is used here because it only takes one sample, and only one
// sample is absolutely needed.  Sample sequencer 3 generates a
// raw interrupt when the conversion is complete, and it is then
// promoted to an ADC0 controller interrupt.  Hardware Timer0A
// triggers the ADC0 conversion at the programmed interval, and
// software handles the interrupt to process the measurement
// when it is complete.
//
// A simpler approach would be to use software to trigger the
// ADC0 conversion, wait for it to complete, and then process the
// measurement.
//
// This initialization function sets up the ADC according to the
// following parameters.  Any parameters not explicitly listed
// below are not modified:
// Timer0A: enabled
// Mode: 32-bit, down counting
// One-shot or periodic: periodic
// Interval value: programmable using 32-bit period
// Sample time is busPeriod*period
// Max sample rate: <=125,000 samples/second
// Sequencer 0 priority: 1st (highest)
// Sequencer 1 priority: 2nd
// Sequencer 2 priority: 3rd
// Sequencer 3 priority: 4th (lowest)
// SS3 triggering event: Timer0A
// SS3 1st sample source: programmable using variable 'channelNum' [0:11]
// SS3 interrupts: enabled and promoted to controller

volatile uint32_t ADCsamples=0; // adc sample counter
uint32_t ADCsamplesMax;       // max number of adc samples to acquire
uint16_t *ADCBufferPointer;     // global pointer for interrupt usage
volatile uint32_t ADCvalue;     // adc data
volatile int ADCstatus=0;  // adc job status
//------------------------------------------------------------------------
// config gpio mux
int ADC_Pin_Config(unsigned int channelNum){
// **** GPIO pin initialization ****
  switch(channelNum){             // 1) activate clock
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:                       //    these are on GPIO_PORTE
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; 
		  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R4) == 0) {} // 2) allow time for clock to stabilize
		  break;
    case 4:
    case 5:
    case 6:
    case 7:                       //    these are on GPIO_PORTD
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3; 
		  while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R3) == 0) {} // 2) allow time for clock to stabilize
		  break;
    case 10:
    case 11:                      //    these are on GPIO_PORTB
      SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;
      while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1) == 0) {} // 2) allow time for clock to stabilize		
		  break;
    default: return 1;              //    0 to 11 are valid channels on the LM4F120
  }
 
	//delay = SYSCTL_RCGCGPIO_R;      // 2) allow time for clock to stabilize
  //delay = SYSCTL_RCGCGPIO_R;
  
	switch(channelNum){
    case 0:                       //      Ain0 is on PE3
      GPIO_PORTE_DIR_R &= ~0x08;  // 3.0) make PE3 input
      GPIO_PORTE_AFSEL_R |= 0x08; // 4.0) enable alternate function on PE3
      GPIO_PORTE_DEN_R &= ~0x08;  // 5.0) disable digital I/O on PE3
      GPIO_PORTE_AMSEL_R |= 0x08; // 6.0) enable analog functionality on PE3
      break;
    case 1:                       //      Ain1 is on PE2
      GPIO_PORTE_DIR_R &= ~0x04;  // 3.1) make PE2 input
      GPIO_PORTE_AFSEL_R |= 0x04; // 4.1) enable alternate function on PE2
      GPIO_PORTE_DEN_R &= ~0x04;  // 5.1) disable digital I/O on PE2
      GPIO_PORTE_AMSEL_R |= 0x04; // 6.1) enable analog functionality on PE2
      break;
    case 2:                       //      Ain2 is on PE1
      GPIO_PORTE_DIR_R &= ~0x02;  // 3.2) make PE1 input
      GPIO_PORTE_AFSEL_R |= 0x02; // 4.2) enable alternate function on PE1
      GPIO_PORTE_DEN_R &= ~0x02;  // 5.2) disable digital I/O on PE1
      GPIO_PORTE_AMSEL_R |= 0x02; // 6.2) enable analog functionality on PE1
      break;
    case 3:                       //      Ain3 is on PE0
      GPIO_PORTE_DIR_R &= ~0x01;  // 3.3) make PE0 input
      GPIO_PORTE_AFSEL_R |= 0x01; // 4.3) enable alternate function on PE0
      GPIO_PORTE_DEN_R &= ~0x01;  // 5.3) disable digital I/O on PE0
      GPIO_PORTE_AMSEL_R |= 0x01; // 6.3) enable analog functionality on PE0
      break;
    case 4:                       //      Ain4 is on PD3
      GPIO_PORTD_DIR_R &= ~0x08;  // 3.4) make PD3 input
      GPIO_PORTD_AFSEL_R |= 0x08; // 4.4) enable alternate function on PD3
      GPIO_PORTD_DEN_R &= ~0x08;  // 5.4) disable digital I/O on PD3
      GPIO_PORTD_AMSEL_R |= 0x08; // 6.4) enable analog functionality on PD3
      break;
    case 5:                       //      Ain5 is on PD2
      GPIO_PORTD_DIR_R &= ~0x04;  // 3.5) make PD2 input
      GPIO_PORTD_AFSEL_R |= 0x04; // 4.5) enable alternate function on PD2
      GPIO_PORTD_DEN_R &= ~0x04;  // 5.5) disable digital I/O on PD2
      GPIO_PORTD_AMSEL_R |= 0x04; // 6.5) enable analog functionality on PD2
      break;
    case 6:                       //      Ain6 is on PD1
      GPIO_PORTD_DIR_R &= ~0x02;  // 3.6) make PD1 input
      GPIO_PORTD_AFSEL_R |= 0x02; // 4.6) enable alternate function on PD1
      GPIO_PORTD_DEN_R &= ~0x02;  // 5.6) disable digital I/O on PD1
      GPIO_PORTD_AMSEL_R |= 0x02; // 6.6) enable analog functionality on PD1
      break;
    case 7:                       //      Ain7 is on PD0
      GPIO_PORTD_DIR_R &= ~0x01;  // 3.7) make PD0 input
      GPIO_PORTD_AFSEL_R |= 0x01; // 4.7) enable alternate function on PD0
      GPIO_PORTD_DEN_R &= ~0x01;  // 5.7) disable digital I/O on PD0
      GPIO_PORTD_AMSEL_R |= 0x01; // 6.7) enable analog functionality on PD0
      break;
    case 8:                       //      Ain8 is on PE5
      GPIO_PORTE_DIR_R &= ~0x20;  // 3.8) make PE5 input
      GPIO_PORTE_AFSEL_R |= 0x20; // 4.8) enable alternate function on PE5
      GPIO_PORTE_DEN_R &= ~0x20;  // 5.8) disable digital I/O on PE5
      GPIO_PORTE_AMSEL_R |= 0x20; // 6.8) enable analog functionality on PE5
      break;
    case 9:                       //      Ain9 is on PE4
      GPIO_PORTE_DIR_R &= ~0x10;  // 3.9) make PE4 input
      GPIO_PORTE_AFSEL_R |= 0x10; // 4.9) enable alternate function on PE4
      GPIO_PORTE_DEN_R &= ~0x10;  // 5.9) disable digital I/O on PE4
      GPIO_PORTE_AMSEL_R |= 0x10; // 6.9) enable analog functionality on PE4
      break;
    case 10:                      //       Ain10 is on PB4
      GPIO_PORTB_DIR_R &= ~0x10;  // 3.10) make PB4 input
      GPIO_PORTB_AFSEL_R |= 0x10; // 4.10) enable alternate function on PB4
      GPIO_PORTB_DEN_R &= ~0x10;  // 5.10) disable digital I/O on PB4
      GPIO_PORTB_AMSEL_R |= 0x10; // 6.10) enable analog functionality on PB4
      break;
    case 11:                      //       Ain11 is on PB5
      GPIO_PORTB_DIR_R &= ~0x20;  // 3.11) make PB5 input
      GPIO_PORTB_AFSEL_R |= 0x20; // 4.11) enable alternate function on PB5
      GPIO_PORTB_DEN_R &= ~0x20;  // 5.11) disable digital I/O on PB5
      GPIO_PORTB_AMSEL_R |= 0x20; // 6.11) enable analog functionality on PB5
      break;
  }
return 0;
}

// config ADC for channelNum
// sequencer 3 with interrupt
void ADC_Init(unsigned int channelNum){
  DisableInterrupts();             // disable interrupt
	SYSCTL_RCGCADC_R |= 0x01;        // activate ADC0 
	while((SYSCTL_PRADC_R&SYSCTL_PRADC_R0)==0){} // allow time to finish activating
	ADC0_PC_R = ADC_PP_MSR_1M;       // configure for 1M samples/sec                      0x07 for 1MSps
  ADC0_SSPRI_R = 0x3210;    // sequencer 0 is highest, sequencer 3 is lowest
  ADC0_ACTSS_R &= ~0x08;    // disable sample sequencer 3
  ADC0_EMUX_R = (ADC0_EMUX_R&0xFFFF0FFF)+0x5000; // timer trigger event
  ADC0_SSMUX3_R = channelNum;
  ADC0_SSCTL3_R = 0x06;          // set flag and end                       
  ADC0_IM_R |= 0x08;             // enable SS3 interrupts
  ADC0_ACTSS_R |= 0x08;          // enable sample sequencer 3	
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00004000; //priority 2
  NVIC_EN0_R = 1<<17;              // enable interrupt 17 in NVIC

}


int ADC_Collect(unsigned int channelNum, unsigned int fs, unsigned short buffer[], unsigned int numberOfSamples){
	ADCsamplesMax=numberOfSamples;                 // max # of samples
	ADCBufferPointer=buffer;                       // save address to global variable
	ADCsamples=0;                                  // make sure we start at 0
	ADCstatus=ADC_STATUS_BUSY;                                   // reset job status to not done

	// config gpio mux. clk to gpio. pin function config
	int status = ADC_Pin_Config(channelNum);
	// config ADC with interrupt.  Timer-triggered ADC
	ADC_Init(channelNum);
	// init timer to trigger adc based on sampling frequency fs
	// given min fs=100 Hz we use prescaler of 12 for the 16-bit timer
	uint32_t 	TIMER_PRESCALER = 0x0C; // prescaler is 12 so that the max time is 80MHz/100Hz/(12+1)=10.65 ms for fs=100 Hz
	Timer0_Init(fs,TIMER_PRESCALER);
  EnableInterrupts();
	return status;
}


// adc job status
int ADC_Status(){return ADCstatus;}

// current value in timer
volatile uint32_t Timer0_Current;
// IRQ 19 handler.  This is for timer0 debugging.  Normally IRQ 19 is disabled

void Timer0A_Handler(void){
  Timer0_Current=TIMER0_TAV_R&(0x0000FFFF); //current free running 16-bit mode	
  TIMER0_ICR_R = TIMER_ICR_TATOCINT; // acknowledge TIMER0A timeout
  GPIO_PORTF_DATA_R |= 0x04;         //led on	                            // interrupt counter
  // turn off timer0 when done. index starts at 0 so stop at N-1
	if(ADCsamples==ADCsamplesMax-1){
		TIMER0_CTL_R = 0x00000000;     // disable timer0
		ADCsamples=0;                  // reset counter
		return;
	}
  ADCsamples++;	
}

// IRQ 17 handler
void ADC0Seq3_Handler(void){
	debug_ledOn(PF2);       // turn on debug led
  debug_ledOff(PF2);
	
	ADC0_ISC_R = 0x08;               // acknowledge ADC sequence 3 completion
	ADCvalue = (ADC0_SSFIFO3_R&0x00000FFF);       // save last 12 bits from 32-bit result
		                               // store result inside a 16-bit buffer
	*(ADCBufferPointer+ADCsamples) = (uint16_t)ADCvalue; 
  ADCsamples++;                    // counter

                                   // if reached quota
	if(ADCsamples == ADCsamplesMax){
		TIMER0_CTL_R = 0x00000000;     // disable timer0
		ADC0_ACTSS_R &= ~0x08;         // disable sample sequencer 3
		//ADCsamples=0;                // reset counter
		ADCstatus=ADC_STATUS_DONE;		 // set flag to 1 indicating job done
		return;                        // exit function
	}
	
}
