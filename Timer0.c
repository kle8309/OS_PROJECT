#include "Timer0.h"
#include "tm4c123gh6pm.h"
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
// initialize timer0 based on sampling frequency
// and timer prescaler (8 bits are LSB bits in count down mode)
// Input: fs sampling frequency
//				prescaler timer prescaler.  new timer period=(prescaler+1)/fbus
// number of cylces to achieve fs is = fbus/(fs(prescaler+1))
// this function calculates the number of cycles automatically base on the inputs
// make sure that the prescaler is large enough to achieve the longest time
// example for 16-bit timer
// 2^16*(prescaler+1)/fbus=10.65ms so this is safe for fs=100 Hz
// Output: N/A
void Timer0_Init(unsigned int fsampling,uint32_t 	prescaler){
	
  uint32_t period = (uint32_t)(80000000/(fsampling*(prescaler+1)));  // period=fclk/(fs*(prescaler+1))
  DisableInterrupts();             // disable interrupt
  SYSCTL_RCGCTIMER_R |= 0x01;      // activate timer0 	
  while((SYSCTL_PRTIMER_R&SYSCTL_PRTIMER_R0)==0){} // allow time to finish activating
	
  TIMER0_CTL_R = 0x00000000;        // disable timer0A during setup
  TIMER0_CTL_R |= TIMER_CTL_TASTALL;//GPTM Timer A Stall Enable	
  TIMER0_CTL_R |= 0x00000020;       // enable timer0A trigger to ADC
  TIMER0_CFG_R =  0x00000004;       // configure for 32-bit timer mode 0x00. use 0x04 for 16-bit mode
  TIMER0_TAMR_R = 0x00000002;       // configure for periodic mode, default down-count settings
  TIMER0_TAPR_R = prescaler;        // prescale value for trigger 12.5ns*(12+1)*2^16=10.65ms (10ms is the min since fs,min=100Hz)
  TIMER0_TAILR_R = period-1;          // start value for trigger
  TIMER0_ICR_R = 0x00000001;        // clear TIMER0A timeout flag
  TIMER0_IMR_R = 0x00000000;        // disable all interrupts
  //TIMER0_IMR_R = 0x00000001;      // arm timeout interrupt
  //NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // 8) priority 4
  //NVIC_EN0_R = 1<<19;              // enable interrupt 17 in NVIC
  TIMER0_CTL_R |= 0x00000001;   // enable timer0A 16-b, periodic, interrupts
}
