#ifndef OS_H
#define OS_H

#include <stdint.h>
#include <stdio.h>

// defines task handler function signature
typedef void (*taskPtr)(void);

// periodic task data structure definition
typedef struct {
    taskPtr task;         // pointer to task
    uint32_t period;      // period in ms
    uint32_t priority;    // 0 is highest
} PeriodicTask;

void OS_InitPeriodicClock(uint32_t period);
void OS_ClearPeriodicTime(void);

uint32_t OS_AddPeriodicThread(void(*task)(void), uint32_t period, uint32_t priority);
uint32_t OS_RemovePeriodicThread(void(*task)(void));
uint32_t OS_ReadPeriodicTime(void);

#endif
