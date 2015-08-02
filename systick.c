#include "systick.h"

void SysTick_Init(uint32_t val){
    // set reload value (1ms if out of bounds)
    if (val <= 0xFFFFFF){
        NVIC_ST_RELOAD_R = val;
    } else {
        NVIC_ST_RELOAD_R = 80000;
    }
        
    // any write to current resets it
    NVIC_ST_CURRENT_R = 0;
    
    // select core clock, enable interrupt, and enable systick = 0x07
    NVIC_ST_CTRL_R = 0x7;
    
    // set priority.  Pg 172 of datasheet Register 74
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
}

void SysTick_SetReload(uint32_t val){
    // set reload value (don't set if out of bounds)
    if (val <= 0xFFFFFF){
        NVIC_ST_RELOAD_R = val;
    }
}

void SysTick_Reset(void){
    // any write to current resets it
    NVIC_ST_CURRENT_R = 0;
}
