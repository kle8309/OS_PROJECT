#ifndef SYSTICK_H
#define SYSTICK_H

#include "tm4c123gh6pm.h"

void SysTick_Init(uint32_t val);

void SysTick_SetReload(uint32_t val);

void SysTick_Reset(void);

#endif
