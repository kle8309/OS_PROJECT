#include "tm4c123gh6pm.h"
#include "debug.h"

void DEBUG_Init(void){
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;               // activate port F
  while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5)==0){};   // allow time for clock to stabilize
    
  GPIO_PORTF_DIR_R |= 0x0E;        // make PF3-1 output (PF3-1 built-in LEDs)
  GPIO_PORTF_AFSEL_R &= ~0x0E;     // disable alt funct on PF3-1
  GPIO_PORTF_DEN_R |= 0x0E;        // enable digital I/O on PF3-1
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFF000F)+0x00000000;
  GPIO_PORTF_AMSEL_R &= ~0x0E;     // disable analog functionality on PF
}
