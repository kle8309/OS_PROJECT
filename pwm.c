#include "tm4c123gh6pm.h"
#include "pwm.h"

#define PWM_CLOCK_FREQ  40000000  // when using /2 divider and 80MHz sys clock
//#define PWM_CLOCK_FREQ  20000000  // when using /4 divider and 80MHz sys clock
//#define PWM_CLOCK_FREQ  10000000  // when using /8 divider and 80MHz sys clock

// private internal state variables
static uint16_t _pwmPeriod;
static uint16_t _dutyPeriod;

void PWM0A_Init(uint16_t period, uint16_t duty){
   SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;   // activate PWM0 clock gating
   while ((SYSCTL_PRPWM_R & SYSCTL_PRPWM_R0) == 0) {}; // wait for PWM0 module to be ready
       
   SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // activate PORTB clock gating
   while ((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R1) == 0) {}; // wait for PORTB to be ready
    
   // configure PB6 as PWM output
   GPIO_PORTB_AFSEL_R |= 0x40;           // enable alt funct on PB6
   GPIO_PORTB_PCTL_R &= ~0x0F000000;     // configure PB6 as PWM0
   GPIO_PORTB_PCTL_R |= 0x04000000;
   GPIO_PORTB_AMSEL_R &= ~0x40;          // disable analog functionality on PB6
   GPIO_PORTB_DEN_R |= 0x40;             // enable digital I/O on PB6
   
   // configure PWM clock divisor
   SYSCTL_RCC_R |= 0x00100000;           // use PWM divider
   SYSCTL_RCC_R = (SYSCTL_RCC_R & (~0x000E0000)) | (0x0<<17);   //    configure for /2 divider
   //SYSCTL_RCC_R = (SYSCTL_RCC_R & (~0x000E0000)) | (0x1<<17);   //    configure for /4 divider
   //SYSCTL_RCC_R = (SYSCTL_RCC_R & (~0x000E0000)) | (0x2<<17);   //    configure for /8 divider

   // configure PWM settings
   PWM0_0_CTL_R = 0;                     // re-loading down-counting mode
   PWM0_0_GENA_R = 0xC8;                 // low on LOAD, high on CMPA down
   
   PWM0A_SetPeriod(period);              // init pwm period
   PWM0A_SetDuty(duty);                  // init pwm duty cycle
}

void PWM0A_Enable(void){
   PWM0_0_CTL_R  |= 0x00000001;          // start PWM0
   PWM0_ENABLE_R |= 0x00000001;          // enable PB6/M0PWM0
}

void PWM0A_Disable(void){
   PWM0_0_CTL_R  &= ~0x00000001;          // stop PWM0
   PWM0_ENABLE_R &= ~0x00000001;          // disable PB6/M0PWM0
}

void PWM0A_SetDuty(uint16_t duty){
   _dutyPeriod = duty;
   PWM0_0_CMPA_R = duty - 1;              // count value when output rises
}

void PWM0A_SetPeriod(uint16_t period){
   _pwmPeriod = period;
   PWM0_0_LOAD_R = period - 1;            // cycles needed to count down to 0
}

void PWM0A_SetFrequency(uint32_t frequency){
   uint16_t newPeriod = PWM_CLOCK_FREQ / frequency;
   uint16_t dutyFactor = (1000 * _dutyPeriod) / _pwmPeriod;
   uint16_t newDuty = (dutyFactor * newPeriod) / 1000;
    
   PWM0A_SetDuty(newDuty);
   PWM0A_SetPeriod(newPeriod);
}

void PWM0A_SetDutyPercent(uint16_t dutyPercent){
   uint16_t newDuty = ( _pwmPeriod * dutyPercent ) / 100;
   
   PWM0A_SetDuty(newDuty);
}

uint32_t PWM0A_GetFrequency(void){
   return (PWM_CLOCK_FREQ / _pwmPeriod);
}
