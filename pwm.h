#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void PWM0A_Init(uint16_t period, uint16_t duty);
void PWM0A_Enable(void);
void PWM0A_Disable(void);
void PWM0A_SetDuty(uint16_t duty);
void PWM0A_SetPeriod(uint16_t period);
void PWM0A_SetFrequency(uint32_t frequency);
uint32_t PWM0A_GetFrequency(void);
void PWM0A_SetDutyPercent(uint16_t dutyPercent);

#endif
